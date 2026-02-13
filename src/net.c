#include "net.h"
#include <asm-generic/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>

//
//   fd
//   port
//

wc_sock* wc_socket_init(int port) {
    wc_sock* ret = (wc_sock*)malloc(sizeof(wc_sock));
    ret->server_addr.sin_family = AF_INET;
    ret->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(port < 0 || port > 0xffff) {
        perror("invalid port");
        exit(EXIT_FAILURE);
    }
    ret->server_addr.sin_port = htons(port);
    return ret;
}

void wc_socket_listen(wc_sock* sock) {
    int optval = 1;
    if((sock->server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(sock->server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setsockopt error");
        exit(EXIT_FAILURE);
    }

    if(bind(sock->server_fd, (struct sockaddr*)&sock->server_addr,sizeof(sock->server_addr)) < 0) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }

    if(listen(sock->server_fd, 5) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }
}

void wc_socket_accept(wc_sock* sock) {
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);

    if((client_fd = accept(sock->server_fd, (struct sockaddr*)&client_addr, &client_addrlen)) < 0) {
        perror("accept error");
        exit(EXIT_FAILURE);
    }

    sock->client_fd = client_fd;
    sock->client_addr = client_addr;

}
