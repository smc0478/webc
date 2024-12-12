
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "socket.h"
#include <unistd.h>
#include <string.h>
char *msg = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConstent-Type: text/html\r\n\r\nab";

int main() {
    char buf[1024];
    int fd = wc_socket_create();
    int reuse = 1;
    struct sockaddr_in address;
    int opt = 1,s;
    socklen_t addrlen = sizeof(address);
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(3000);

    if(bind(fd, (struct sockaddr*)&address,sizeof(address)) < 0) {
        perror("bind fail");
        exit(EXIT_FAILURE);
    }

    if (listen(fd, 5) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    if ((s = accept(fd, (struct sockaddr*)&address,
                      &addrlen))
            < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
    }
    read(s, buf,1024 - 1);
    printf("test\n");
    printf("%s\n",buf);
    send(s,msg,strlen(msg),0);
    printf("test\n");

    close(s);
    close(fd);
    return 0;
}
