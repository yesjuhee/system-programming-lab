PA3 Report

---

# Socket Programming

서버와 클라이언트의 통신을 위해 TCP Socket 을 이용했다.

## Server

```c
int listen_fd = -1;

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
```

서버의 `socket()` → `bind()` → `listen()` 까지를 담당하는 함수. `accept()` 이후의 부분은 이후에 다룬다.

## Client

```c
int conn_fd = -1;
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
```

# Booking Services

- 서버와 클라이언트의 연결이 끝나면 스켈레톤 코드에 제공된 `Request` `Response` 구조체를 이용해 통신한다.
- 클라이언트는 유저에게 [USER, ACTION, DATA] 형식으로 표준 입력을 이용해 입력을 받는다.
- 서버는 `Request`의 `action` 에 따라 미리 구현된 서비스를 수행하고 클라이언트에게 `Response`를 보낸다.

## Client

```c
void switch_action(struct Request *req, struct Response *res);
void action_termination(struct Request *req, struct Response *res);
void action_login(struct Request *req, struct Response *res);
void action_logout(struct Request *req, struct Response *res);
void action_book(struct Request *req, struct Response *res);
void action_confirm_booking(struct Request *req, struct Response *res);
void action_cancel_booking(struct Request *req, struct Response *res);

int main(int argc, char *argv[]) {
	...
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
	...
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
```

클라이언트에서 `readline`으로 입력을 받고 입력에 따라 함수를 수행하는 부분. 각 `action_` 함수에서는 요청을 검토하고, 보내고, 응답을 판단하여 알맞은 메시지를 출력한다.

## Server

```c
void action_termination(struct Request *req, struct Response *res);
void action_login(struct Request *req, struct Response *res);
void action_logout(struct Request *req, struct Response *res);
void action_book(struct Request *req, struct Response *res);
void action_confirm_booking(struct Request *req, struct Response *res);
void action_cancel_booking(struct Request *req, struct Response *res);

void *thread_func() {
				...
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
        ...
}
```

서버도 클라이언트와 마찬가지로 `switch`문과 `action_` 함수를 이용하여 요청을 처리하고 알맞은 응답을 보내도록 구현되었다.

# Threading Pool

```c
int *queue[MAX_QUEUE];
int queue_size = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[]){
		...
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
    ...
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
        // Do tasks
        ...
	      }
    }
    return NULL;
}
```

서버에 여러 유저가 동시 접속이 가능하도록 Thread Pool을 이용하였다.

# Data Structure

```c
struct BookingInfo {
    int booked;
    uint32_t user;
};
uint8_t *active_user = NULL;
size_t active_user_len = 4096;
struct BookingInfo booked_seat[MAX_SEAT + 1] = { 0 };

void init_active_user_array();
void resize_active_user_array(uint32_t user_id);
```

로그인 유저를 관리하기 위해 `active_user` 배열을, book 정보를 관리하기 위해 `BookingInfo` 구조체를 위와 같이 사용하였다. 유저의 아이디의 범위가 큰 값으로 들어와도 문제 없이 작동할 수 있도록 `malloc`과 `realloc`을 이용해 `active_user` 배열의 크기를 조절하는 방식으로 구현했다.
