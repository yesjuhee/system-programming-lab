#include <argon2.h> // hashing
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define MEMORY_USAGE 512
#define SALT_SIZE 16
#define HASH_SIZE 32
#define HASHED_PASSWORD_SIZE 128

#define MAX_QUEUE 4096
#define MAX_SEAT 256
#define MAX_DATA MAX_SEAT * 8

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
struct BookingInfo {
    int booked;
    uint32_t user;
};

int listen_fd = -1;

uint8_t *active_user = NULL;
size_t active_user_len = 4096;
struct BookingInfo booked_seat[MAX_SEAT + 1] = { 0 };

int *queue[MAX_QUEUE];
int queue_size = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

void handle_exit();
void setup_listen_fd(in_port_t port);
int get_num_cores();

int recv_request(int fd, struct Request *req);
void send_response(int fd, struct Response *res);

void generate_salt(uint8_t *salt);
void hash_password(char *password, char *hashed_pasword);
int validate_password(char *password_to_validate, char *hashed_password);
void init_active_user_array();
void resize_active_user_array(uint32_t user_id);

void action_termination(struct Request *req, struct Response *res);
void action_login(struct Request *req, struct Response *res);
void action_logout(struct Request *req, struct Response *res);
void action_book(struct Request *req, struct Response *res);
void action_confirm_booking(struct Request *req, struct Response *res);
void action_cancel_booking(struct Request *req, struct Response *res);

void *thread_func();

int main(int argc, char *argv[]) {
    atexit(handle_exit);

    if (argc < 2) {
        fprintf(stderr, "Received %d arguments. Please enter port number!\n", argc - 1);
        exit(1);
    }
    in_port_t port = (in_port_t)strtol(argv[1], NULL, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "invalid port number %s\n", argv[1]);
        exit(1);
    }

    init_active_user_array();
    setup_listen_fd(port);
    socklen_t conn_addr_len;
    struct sockaddr_in conn_addr;

    int *connfdp;

    int thread_pool_size = get_num_cores();
    pthread_t *tid = (pthread_t *)malloc(sizeof(pthread_t) * thread_pool_size);
    for (int i = 0; i < thread_pool_size; i++) {
        pthread_create(&tid[i], NULL, thread_func, NULL);
    }

    while (1) {
        puts("Waiting for connection");
        connfdp = (int *)malloc(sizeof(int));
        /* Accept connection request from clients */
        conn_addr_len = sizeof(conn_addr);
        if ((*connfdp = accept(listen_fd, (struct sockaddr *)&conn_addr, &conn_addr_len)) < 0) {
            fprintf(stderr, "accept() failed.\n");
            continue;
        }

        puts("Connected!");
        pthread_mutex_lock(&queue_mutex);
        queue[queue_size++] = connfdp; // add client connection to thread pool
        pthread_cond_signal(&queue_cond);
        pthread_mutex_unlock(&queue_mutex);
    }

    close(listen_fd);
    free(tid);
    free(connfdp);
    free(active_user);
}

void *thread_func() {
    while (1) {
        pthread_mutex_lock(&queue_mutex);
        while (queue_size == 0) { // if queue is empty
            // sleep until other threads signals the condition value(queue_cond)
            // mutex is automatically released
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        int *connfdp;
        connfdp = queue[0]; // new task
        memmove(queue, queue + 1, sizeof(int *) * --queue_size);
        pthread_mutex_unlock(&queue_mutex);

        while (1) {
            struct Request request;
            struct Response response;
            request.data = NULL;
            response.data = NULL;
            errno = 0;

            // recv request
            if (recv_request(*connfdp, &request) < 0) {
                close(*connfdp);
                free(connfdp);
                break;
            }

            switch (request.action) {
            case 0: // Termination
                action_termination(&request, &response);
                send_response(*connfdp, &response);
                break;
            case 1: // Login
                action_login(&request, &response);
                send_response(*connfdp, &response);
                break;
            case 2: // Book
                action_book(&request, &response);
                send_response(*connfdp, &response);
                break;
            case 3: // Confirm Booking
                action_confirm_booking(&request, &response);
                send_response(*connfdp, &response);
                break;
            case 4: // Cancel Booking
                action_cancel_booking(&request, &response);
                send_response(*connfdp, &response);
                break;
            case 5: // Logout
                action_logout(&request, &response);
                send_response(*connfdp, &response);
                break;
            default:
                fprintf(stderr, "Action %d is unknown.\n", request.action);
            }

            if (request.data) {
                free(request.data);
            }
            if (response.data) {
                free(response.data);
            }
        }
    }
    return NULL;
}

void init_active_user_array() {
    active_user = (uint8_t *)malloc(active_user_len * sizeof(uint8_t));
    memset(active_user, 0, active_user_len * sizeof(uint8_t));
}

void resize_active_user_array(uint32_t user_id) {
    if (user_id >= active_user_len) {
        size_t new_length = active_user_len * 2;
        while (new_length <= user_id) {
            new_length *= 2;
        }
        uint8_t *new_array = (uint8_t *)realloc(active_user, new_length * sizeof(uint8_t));
        if (!new_array) {
            fprintf(stderr, "realloc() failed.\n");
            free(active_user);
            exit(1);
        }
        memset(new_array + active_user_len, 0, (new_length - active_user_len) * sizeof(uint8_t));
        active_user = new_array;
        active_user_len = new_length;
    }
}

int get_num_cores() {
    cpu_set_t cpu_set;
    sched_getaffinity(0, sizeof(cpu_set), &cpu_set);
    return CPU_COUNT_S(sizeof(cpu_set), &cpu_set);
}

void generate_salt(uint8_t *salt) {
    int fd = open("/dev/urandom", O_RDONLY);
    read(fd, salt, SALT_SIZE);
    close(fd);
}

void hash_password(char *password, char *hashed_pasword) {
    uint8_t salt[SALT_SIZE];
    generate_salt(salt);
    char hash[HASHED_PASSWORD_SIZE];
    argon2id_hash_encoded(2, MEMORY_USAGE, 1, password, strlen(password), salt, SALT_SIZE, HASH_SIZE, hash, HASHED_PASSWORD_SIZE);
    strcpy(hashed_pasword, hash);
}

int validate_password(char *password_to_validate, char *hashed_password) {
    if (argon2id_verify(hashed_password, password_to_validate, strlen(password_to_validate)) == ARGON2_OK) {
        return 1;
    } else {
        return 0;
    }
}

void handle_exit() {
    if (listen_fd != -1) {
        close(listen_fd);
    }
    free(active_user);
}

void setup_listen_fd(in_port_t port) {
    struct sockaddr_in sock_addr;

    /* Create listen socket */
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fprintf(stderr, "socket() failed\n");
        exit(1);
    }

    /* Bind sockaddr (IP, etc.) to listen socket */
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_port = htons(port);
    if (bind(listen_fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0) {
        fprintf(stderr, "bind() failed.\n");
        exit(2);
    }

    /* Listen to listen socket */
    if (listen(listen_fd, MAX_QUEUE) < 0) {
        fprintf(stderr, "listen() failed.\n");
        exit(3);
    }
}

int recv_request(int fd, struct Request *req) {
    uint8_t header[sizeof(req->user) + sizeof(req->size) + sizeof(req->action)];
    if (read(fd, header, sizeof(header)) == 0) {
        return -1;
    }

    // Receive user, size, action
    memcpy(&(req->user), header, sizeof(req->user));
    memcpy(&(req->size), header + sizeof(req->user), sizeof(req->size));
    memcpy(&(req->action), header + sizeof(req->user) + sizeof(req->size), sizeof(req->action));

    // Receive data
    if (req->size == 0) {
        req->data = NULL;
        return 0;
    }
    req->data = (uint8_t *)malloc(req->size);
    if (read(fd, req->data, req->size) == 0) {
        return -1;
    }
    return 0;
}

void send_response(int fd, struct Response *res) {
    size_t total_size = sizeof(res->code) + sizeof(res->size) + res->size;
    uint8_t *buffer = malloc(total_size);

    memcpy(buffer, &(res->code), sizeof(res->code));
    memcpy(buffer + sizeof(res->code), &(res->size), sizeof(res->size));
    memcpy(buffer + sizeof(res->code) + sizeof(res->size), res->data, res->size);

    write(fd, buffer, total_size);

    fprintf(stdout, "Sent %ld bytes response\n", total_size);

    free(buffer);
}

void action_termination(struct Request *req, struct Response *res) {
    if (!req->data) {
        res->code = 1;
    } else {
        long data = strtol((char *)req->data, NULL, 10);
        if (errno == ERANGE || data != 0) {
            res->code = 1;
        } else {
            res->code = 0;
        }
    }
    res->size = 0;
    res->data = NULL;
}

void action_login(struct Request *req, struct Response *res) {
    uint32_t user_id = req->user;
    char *password = (char *)req->data;
    int stored_user_id = 0;
    char stored_hashed_password[HASHED_PASSWORD_SIZE];
    int user_exists = 0;

    resize_active_user_array(user_id);

    if (active_user[user_id] == 1) {
        // user is active
        res->code = 1;
        res->size = 0;
        res->data = NULL;
        return;
    }

    FILE *file = fopen("/tmp/password.tsv", "a+");
    if (!file) {
        fprintf(stderr, "fopen");
        res->code = 1;
        return;
    }

    // Check if user exists
    while (fscanf(file, "%d\t%s\n", &stored_user_id, stored_hashed_password) > 0) {
        if (stored_user_id == user_id) {
            user_exists = 1;
            break;
        }
    }

    if (user_exists) {
        // Validate password
        if (validate_password(password, stored_hashed_password)) {
            active_user[user_id] = 1;
            res->code = 0; // Success
        } else {
            res->code = 3; // Invalid password
        }
    } else {
        // Register new user
        char new_hashed_password[HASHED_PASSWORD_SIZE];
        hash_password(password, new_hashed_password);
        fprintf(file, "%u\t%s\n", user_id, new_hashed_password);
        active_user[user_id] = 1;
        res->code = 0; // Suceess
    }

    fclose(file);
    res->size = 0;
    res->data = NULL;
}

void action_logout(struct Request *req, struct Response *res) {
    uint32_t user_id = req->user;
    if (user_id >= active_user_len) {
        res->code = 1; // User not logged in
    } else if (active_user[user_id] == 1) {
        res->code = 0;
        active_user[user_id] = 0;
    } else {
        res->code = 1;
    }
    res->size = 0;
    res->data = NULL;
}

void action_book(struct Request *req, struct Response *res) {
    uint32_t user_id = req->user;
    if (active_user[user_id] == 0) { // User is not logged in
        res->code = 1;
        res->data = NULL;
        res->size = 0;
        return;
    }

    long seat_number = strtol((char *)req->data, NULL, 10);
    if (errno == ERANGE || seat_number < 1 || seat_number > MAX_SEAT) { // Seat number is out of range
        res->code = 3;
        res->data = NULL;
        res->size = 0;
        return;
    }
    if (booked_seat[seat_number].booked == 1) { // Seat is unavailable
        res->code = 2;
        res->data = NULL;
        res->size = 0;
        return;
    }

    // Book Success
    res->code = 0;
    res->size = req->size;
    res->data = (uint8_t *)malloc(res->size);
    memcpy(res->data, req->data, req->size);
    booked_seat[seat_number].booked = 1;
    booked_seat[seat_number].user = user_id;
}

void action_confirm_booking(struct Request *req, struct Response *res) {
    uint32_t user_id = req->user;
    if (active_user[user_id] == 0) { // User is not logged in
        res->code = 1;
        res->data = NULL;
        res->size = 0;
        return;
    }

    res->code = 0;
    char buffer[MAX_DATA] = { 0 };
    int length = 0;
    if (req->size == 0) {
        // Confirm seats booked by the user
        for (int i = 1; i <= MAX_SEAT; i++) {
            if (booked_seat[i].booked && booked_seat[i].user == user_id) {
                length += snprintf(buffer + length, MAX_DATA - length, "%d, ", i);
            }
        }
    } else {
        // Confirm all available seats
        for (int i = 1; i <= MAX_SEAT; i++) {
            if (booked_seat[i].booked == 0) {
                length += snprintf(buffer + length, MAX_DATA - length, "%d, ", i);
            }
        }
    }

    if (length == 0) { // No matching seat
        res->size = 0;
        res->code = 0;
        res->data = NULL;
    } else {
        // Remove the last comma
        buffer[length - 2] = '\0';
        length--;
        res->size = length;
        res->data = (uint8_t *)malloc(res->size);
        memcpy(res->data, buffer, res->size);
    }
}

void action_cancel_booking(struct Request *req, struct Response *res) {
    uint32_t user_id = req->user;
    if (active_user[user_id] == 0) { // User is not logged in
        res->code = 1;
        res->data = NULL;
        res->size = 0;
        return;
    }

    long seat_number = strtol((char *)req->data, NULL, 10);
    if (errno == ERANGE || seat_number < 1 || seat_number > MAX_SEAT) { // Seat number is out of range
        res->code = 3;
        res->data = NULL;
        res->size = 0;
        return;
    }

    if (booked_seat[seat_number].booked == 0 || booked_seat[seat_number].user != user_id) { // User did  not book the specified seat
        res->code = 2;
        res->data = NULL;
        res->size = 0;
        return;
    }

    // Cancel booking success
    res->code = 0;
    res->size = req->size;
    res->data = (uint8_t *)malloc(res->size);
    memcpy(res->data, req->data, req->size);
    booked_seat[seat_number].booked = 0;
    booked_seat[seat_number].user = 0;
}
