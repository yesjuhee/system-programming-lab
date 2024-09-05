#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct Request {
    uint32_t user;
    uint32_t size;
    uint8_t action;
    uint8_t *data;
};

struct Response {
    uint32_t code;
    uint32_t size;
    uint8_t *data;
};

int conn_fd = -1;
uint32_t login_id = 0;

void handle_exit();
void setup_conn_fd(char *host, in_port_t port);
int setup_request(struct Request *request, char *input);
void send_request(int fd, struct Request *request);
int recv_response(int fd, struct Response *res);

void switch_action(struct Request *req, struct Response *res);
void action_termination(struct Request *req, struct Response *res);
void action_login(struct Request *req, struct Response *res);
void action_logout(struct Request *req, struct Response *res);
void action_book(struct Request *req, struct Response *res);
void action_confirm_booking(struct Request *req, struct Response *res);
void action_cancel_booking(struct Request *req, struct Response *res);

int main(int argc, char *argv[]) {
    atexit(handle_exit);

    if (argc < 3) {
        fprintf(
            stderr,
            "Received %d arguments. Please enter host address and port number!\n",
            argc - 1);
        exit(1);
    }

    in_port_t port = (in_port_t)strtol(argv[2], NULL, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "invalid port number '%s'\n", argv[2]);
        exit(1);
    }

    setup_conn_fd(argv[1], port);

    if (argc == 4) {
        // File Request
        FILE *file = fopen(argv[3], "r");
        if (!file) {
            fprintf(stderr, "fopen() failed.\n");
            exit(1);
        }

        char line[256];
        while (fgets(line, sizeof(line), file)) {
            int len = strlen(line);
            line[len - 1] = '\0';

            struct Request request;
            struct Response response;

            if (setup_request(&request, line) == 1) {
                continue;
            }

            switch_action(&request, &response);
        }

        // Termination
        struct Request termination_request;
        struct Response termination_response;
        char *command = malloc(6);
        strcpy(command, "0 0 0");
        setup_request(&termination_request, command);
        action_termination(&termination_request, &termination_response);
        free(command);
        fclose(file);
    } else {
        // Interactive Send Request
        while (1) {
            struct Request request = { .data = NULL };
            struct Response response = { .data = NULL };

            char *input = readline("[USER ACTION DATA]: ");
            add_history(input);
            if (setup_request(&request, input) == 1) {
                continue;
            }

            switch_action(&request, &response);

            if (request.action == 0 && response.code == 0) {
                break;
            }
        }
    }

    close(conn_fd);
    conn_fd = -1;

    return 0;
}

void handle_exit() {
    if (conn_fd != -1) {
        close(conn_fd);
    }
}

void setup_conn_fd(char *host, in_port_t port) {
    struct hostent *host_entry;
    struct sockaddr_in sock_addr;

    /* Create socket */
    if ((conn_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fprintf(stderr, "socket() failed.\n");
        exit(2);
    }
    /* Get host information */
    if ((host_entry = gethostbyname(host)) == NULL) {
        fprintf(stderr, "invalid hostname %s\n", host);
        exit(3);
    }
    /* Initialize sock_addr */
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    memcpy(&sock_addr.sin_addr.s_addr, host_entry->h_addr_list[0], host_entry->h_length);
    sock_addr.sin_port = htons(port);
    /* Connect to host */
    if (connect(conn_fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0) {
        fprintf(stderr, "connect() failed.\n");
        exit(4);
    }
}

int setup_request(struct Request *request, char *input) {
    char *save_ptr;
    char *user_id = strtok_r(input, " ", &save_ptr);
    char *action = strtok_r(NULL, " ", &save_ptr);
    char *data = strtok_r(NULL, " ", &save_ptr);

    // set action
    if (action == NULL) {
        fprintf(stderr, "ACTION is not given.\n");
        return 1;
    }
    errno = 0;
    long action_num = strtol(action, NULL, 10);
    if (errno == ERANGE || action_num < 0) {
        fprintf(stderr, "invalid action '%s'\n", action);
        return 1;
    }
    request->action = (uint8_t)action_num;

    // set user id
    if (user_id == NULL) {
        fprintf(stderr, "USER is not given.\n");
        return 1;
    }
    errno = 0;
    long user_id_num = strtol(user_id, NULL, 10);
    if (errno == ERANGE || (action_num != 0 && user_id_num == 0) || user_id_num < 0) {
        fprintf(stderr, "invalid user id '%s'\n", user_id);
        return 1;
    }
    request->user = (uint32_t)user_id_num;

    // set data
    if (data == NULL) {
        request->data = NULL;
        request->size = 0;
    } else {
        request->data = (uint8_t *)data;
        request->size = strlen(data) + 1;
    }

    return 0;
}

void send_request(int fd, struct Request *req) {
    size_t total_size = sizeof(req->user) + sizeof(req->size) + sizeof(req->action) + req->size;
    uint8_t *buffer = malloc(total_size);

    memcpy(buffer, &(req->user), sizeof(req->user));
    memcpy(buffer + sizeof(req->user), &(req->size), sizeof(req->size));
    memcpy(buffer + sizeof(req->user) + sizeof(req->size), &(req->action), sizeof(req->action));
    memcpy(buffer + sizeof(req->user) + sizeof(req->size) + sizeof(req->action), req->data, req->size);

    write(fd, buffer, total_size);

    fprintf(stdout, "Sent %ld bytes request\n", total_size);

    free(buffer);
}

int recv_response(int fd, struct Response *res) {
    uint8_t header[sizeof(res->code) + sizeof(res->size)];
    if (read(fd, header, sizeof(header)) == 0) {
        return -1;
    }

    // Receive code, size
    memcpy(&(res->code), header, sizeof(res->code));
    memcpy(&(res->size), header + sizeof(res->code), sizeof(res->size));

    // Receive data
    if (res->size == 0) {
        res->data = NULL;
        return 0;
    }
    res->data = (uint8_t *)malloc(res->size);
    if (read(fd, res->data, res->size) == 0) {
        return -1;
    }
    return 0;
}

void switch_action(struct Request *req, struct Response *res) {
    switch (req->action) {
    case 0: // Termination
        action_termination(req, res);
        break;
    case 1: // Login
        action_login(req, res);
        break;
    case 2: // Book
        action_book(req, res);
        break;
    case 3: // Confirm Booking
        action_confirm_booking(req, res);
        break;
    case 4: // Cancel Booking
        action_cancel_booking(req, res);
        break;
    case 5: // Log out
        action_logout(req, res);
        break;
    default:
        fprintf(stderr, "Action %d is unknown.\n", req->action);
    }
}

void action_termination(struct Request *req, struct Response *res) {
    if (login_id > 0) {
        // a user is currently logged in
        struct Request logout_requst;
        struct Response logout_response;

        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%d 5", login_id);
        setup_request(&logout_requst, buffer);

        // send logout request first
        send_request(conn_fd, &logout_requst);
        recv_response(conn_fd, &logout_response);
        fprintf(stdout, "Logged out successfully.\n");
    }
    send_request(conn_fd, req);
    recv_response(conn_fd, res);
    if (res->code == 0) {
        fprintf(stdout, "Connection terminated.\n");
    } else if (res->code == 1) {
        fprintf(stderr, "Failed to disconnect as arguments are invalid.\n");
    }
}

void action_login(struct Request *req, struct Response *res) {
    if (login_id > 0) { // active client
        fprintf(stderr, "Failed to log in as client is active.\n");
    }
    send_request(conn_fd, req);
    recv_response(conn_fd, res);
    if (res->code == 0) {
        fprintf(stdout, "Logged in successfully.\n");
        login_id = req->user;
    } else if (res->code == 1) {
        fprintf(stderr, "Failed to log in as user is active.\n");
    } else if (res->code == 2) {
        fprintf(stderr, "Failed to log in as client is active.\n");
    } else if (res->code == 3) {
        fprintf(stderr, "Failed to log in as password is incorrect.\n");
    }
}

void action_book(struct Request *req, struct Response *res) {
    send_request(conn_fd, req);
    recv_response(conn_fd, res);
    if (res->code == 0) {
        fprintf(stdout, "Booked seat %s\n", res->data);
    } else if (res->code == 1) {
        fprintf(stderr, "Failed to book as user is not logged in.\n");
    } else if (res->code == 2) {
        fprintf(stderr, "Failed to book as seat is unavailable.\n");
    } else if (res->code == 3) {
        fprintf(stderr, "Failed to book as seat number is out of range.\n");
    }
}

void action_confirm_booking(struct Request *req, struct Response *res) {
    send_request(conn_fd, req);
    recv_response(conn_fd, res);
    if (res->code == 0) {
        if (req->size == 0) {
            // Confirm booked seat numbers
            if (res->size == 0) {
                fprintf(stdout, "Did not book any seats.\n");
            } else {
                fprintf(stdout, "Booked the seats %s.\n", res->data);
            }
        } else {
            // Confirm available seat numbers
            if (res->size == 0) {
                fprintf(stdout, "No available seats.\n");
            } else {
                fprintf(stdout, "Available the seats %s.\n", res->data);
            }
        }
    } else if (res->code == 1) {
        fprintf(stderr, "Failed to confirm booking as user is not logged in.\n");
    }
}

void action_cancel_booking(struct Request *req, struct Response *res) {
    send_request(conn_fd, req);
    recv_response(conn_fd, res);
    if (res->code == 0) {
        fprintf(stdout, "Canceled seat number %s\n", res->data);
    } else if (res->code == 1) {
        fprintf(stderr, "Failed to cancel booking as user is not logged in.\n");
    } else if (res->code == 2) {
        fprintf(stderr, "Failed to cancel booking as user did not book the specified seat.\n");
    } else if (res->code == 3) {
        fprintf(stderr, "Failed to cancel booking as seat number is out of range.\n");
    }
}

void action_logout(struct Request *req, struct Response *res) {
    send_request(conn_fd, req);
    recv_response(conn_fd, res);
    if (res->code == 0) {
        fprintf(stdout, "Logged out successfully.\n");
        login_id = 0;
    } else if (res->code == 1) {
        fprintf(stderr, "Failed to log out as user is not logged in.\n");
    }
}
