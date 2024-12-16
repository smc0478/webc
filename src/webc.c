#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "webc.h"
#include "dict.h"
#include "net.h"
#include "request.h"
#include "response.h"
#include "http.h"

wc wc_server;

static inline methods get_method_id(char *method) {
    for(methods i = GET; i < OTHERS; i++) {
        if(strcmp(method, method_str[i]) == 0)
            return i;
    }

    return OTHERS;
}

wc* wc_alloc() {
    wc* webc = (wc*)malloc(sizeof(wc));

    return webc;
}

void wc_init(wc* wc, int port) {
    wc->server = wc_socket_init(port);
    for(int i = 0; i < MAX_METHOD_LENGTH; i++)
        dict_init(&wc->routes[i]);
}

void wc_route_free(wc_route* route) {
    free(route->method);
    free(route->path);
    free(route);
}

void wc_add_route(wc* wc, char *method, char *path, wc_handler f) {
    methods id;
    id = get_method_id(method);
    wc_route* route = (wc_route*)malloc(sizeof(wc_route));
    route->method = strdup(method);
    route->path = strdup(path);
    route->f = f;
    dict_insert(&wc->routes[id], path, route, (free_fp)wc_route_free);
}


wc_route* wc_get_route(wc* wc, wc_req* req) {
    methods method = get_method_id(wc_request_get_method(req));
    wc_route* ret = (wc_route*)dict_find_data(&wc->routes[method], req->path);

    return ret;
}

void wc_http_read(wc* wc) {
    char buf[1024*2];               // 2MB
    wc_sock* sock = wc->server;
    int nbytes;

    nbytes = read(sock->client_fd, buf, sizeof(buf)-1);
    if(buf[nbytes] != '\0')
        buf[nbytes] = '\0';

    wc->request = wc_parse_request(buf);
    size_t req_len = strlen(wc->request->raw);
    printf("%s %s\n", wc->request->method, wc->request->path);

}

void wc_http_write(wc* wc) {
    int client_fd = wc->server->client_fd;
    wc_resp* res = wc->response;
    dict header = res->headers;

    FILE* res_write = fdopen(client_fd, "w");

    // method path protocol
    fprintf(res_write, "HTTP/1.0 %d OK\r\n",res->status);
    for(int i = 0; i < header.capacity; i++)
        if(header.bucket[i].state == USED)
            fprintf(res_write, "%s: %s\r\n", header.bucket[i].key, header.bucket[i].value);


    fprintf(res_write, "\r\n");
    fprintf(res_write, "%s", res->body);

    fflush(res_write);
}

void wc_handle_route(wc *wc, wc_route *route) {
    wc->response = route->f(wc->request);
}

void wc_server_init(int port) {
    wc* wc = &wc_server;
    wc_init(wc, port);

}

void wc_server_add_route(char* method, char* path, wc_handler f) {
    wc* wc = &wc_server;
    wc_add_route(wc, method, path, f);
}

void wc_get(char* path, wc_handler f) {
    wc_server_add_route("GET", path, f);
}
void wc_post(char* path, wc_handler f) {
    wc_server_add_route("POST", path, f);
}


void wc_server_start() {
    wc* wc = &wc_server;

    wc_socket_listen(wc->server);

    while(1){
        wc_socket_accept(wc->server);

        wc_http_read(wc);

        wc_route* route = wc_get_route(wc, wc->request);
        wc_handle_route(wc, route);

        wc_http_write(wc);
        close(wc->server->client_fd);
    }
}
