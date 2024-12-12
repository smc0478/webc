#ifndef _NET_H_
#define _NET_H_

//#define MAX_FD 10

//#include <sys/socket.h>
#include <netinet/in.h>

struct _wc_sock {
    int server_fd;
    struct sockaddr_in server_addr;
    int client_fd;
    struct sockaddr_in client_addr;
};

typedef struct _wc_sock wc_sock;

wc_sock* wc_socket_init(int port);
void wc_socket_listen(wc_sock* sock);
void wc_socket_accept(wc_sock* sock);

#endif // net.h
