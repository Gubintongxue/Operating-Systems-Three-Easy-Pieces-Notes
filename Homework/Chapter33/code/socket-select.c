//
// Created by pjs on 2020/11/24.
//

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>
#include "common.h"


#define MAX_MSG 512
#define PORT 8080

void *server();

void *client();

void read_from(int fd);

void send_to(int fd, char *message);

int main() {

    pthread_t s, c[10];
    Pthread_create(&s, NULL, server, NULL);
//    Pthread_create(&c[0], NULL, client, NULL);

    for (int i = 0; i < 10; ++i) {
        Pthread_create(&c[i], NULL, client, NULL);
    }
    for (int i = 0; i < 10; ++i) {
        Pthread_join(c[i], NULL);
    }
    Pthread_join(s, NULL);
//    Pthread_join(c[0], NULL);
    return 0;
}

_Noreturn void *server() {
    fd_set read_fds;
    int socket_fd, new_socket_fd;
    sockaddr_in_t info, client_info;
    socklen_t addrLen = sizeof(client_info);

    // 初始化 info
    init_sockaddr_in(&info, PORT);
    FD_ZERO(&read_fds);

    // 创建套接字并调用 bind 绑定
    socket_fd = make_server_socket(info);
    FD_SET(socket_fd, &read_fds);

    if (listen(socket_fd, SOMAXCONN) < 0) {
        err_exit("listen");
    }

    while (1) {
        if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) < 0) {
            err_exit("select");
        }
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == socket_fd) {
                    //新连接, socket_fd 处于非阻塞模式时, accept不会阻塞
                    new_socket_fd = accept(socket_fd, (struct sockaddr *) &client_info, &addrLen);
                    if (new_socket_fd < 0) {
                        err_exit("accept");
                    }
                    FD_SET(new_socket_fd, &read_fds);
                    printf("connect\n");
                } else {
                    char message[] = "Server: Hi, this is server.\n";
                    read_from(i);
                    send_to(i, message);

                    close(i);
                    FD_CLR(i, &read_fds);
                }
            }
        }

    }
}

void *client() {
    sockaddr_in_t info;
    init_sockaddr_in(&info, PORT);

    char message[] = "Client: Hi there\n";
    int socket_fd = make_socket();
    int error = connect(socket_fd, (struct sockaddr *) &info, sizeof(info));

    if (error < 0) {
        err_exit("client connect\n");
    }

    send_to(socket_fd, message);
    read_from(socket_fd);
    close(socket_fd);
    return NULL;
}

void read_from(int fd) {
    char buf[MAX_MSG];
    int n_bytes = read(fd, buf, MAX_MSG);
    if (n_bytes < 0) {
        err_exit("read");
    } else {
        printf("%s\n", buf);
    }
}

void send_to(int fd, char *message) {
    int size = sizeof(char) * (strlen(message) + 1);
    if (write(fd, message, size) != size) {
        err_exit("write");
    }
}
