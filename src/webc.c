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
    for (methods i = GET; i < OTHERS; i++) {
        if (strcmp(method, method_str[i]) == 0)
            return i;
    }

    return OTHERS;
}

static const char* get_reason_phrase(int status) {
    switch (status) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default: return "Unknown";
    }
}

wc* wc_alloc() {
    wc* webc = (wc*)calloc(1, sizeof(wc));
    return webc;
}

void wc_init(wc* wc, int port) {
    wc->server = wc_socket_init(port);
    for (int i = 0; i < MAX_METHOD_LENGTH; i++)
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

int wc_http_read(wc* wc) {
    char buf[1024 * 2];
    wc_sock* sock = wc->server;
    int nbytes;

    nbytes = read(sock->client_fd, buf, sizeof(buf) - 1);
    if (nbytes <= 0)
        return -1;

    buf[nbytes] = '\0';
    wc->request = wc_parse_request(buf);

    if (wc->request == NULL || wc->request->method == NULL || wc->request->path == NULL)
        return -1;

    printf("%s %s\n", wc->request->method, wc->request->path);
    return 0;
}

void wc_http_write(wc* wc) {
    int client_fd = wc->server->client_fd;
    wc_resp* res = wc->response;
    dict header = res->headers;

    FILE* res_write = fdopen(client_fd, "w");
    if (res_write == NULL) {
        close(client_fd);
        return;
    }

    fprintf(res_write, "HTTP/1.0 %d %s\r\n", res->status, get_reason_phrase(res->status));
    for (int i = 0; i < header.capacity; i++)
        if (header.bucket[i].state == USED)
            fprintf(res_write, "%s: %s\r\n", header.bucket[i].key, (char*)header.bucket[i].value);


    fprintf(res_write, "\r\n");
    fprintf(res_write, "%s", res->body ? res->body : "");

    fflush(res_write);
    fclose(res_write);
}

void wc_handle_route(wc *wc, wc_route *route) {
    if (route == NULL) {
        wc->response = wc_response(404, "<h1>404 Not Found</h1>");
        return;
    }

    wc->response = route->f(wc->request);
    if (wc->response == NULL)
        wc->response = wc_response(500, "<h1>500 Internal Server Error</h1>");
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

    while (1) {
        wc_socket_accept(wc->server);

        if (wc_http_read(wc) != 0) {
            wc->response = wc_response(400, "<h1>400 Bad Request</h1>");
            wc_http_write(wc);
            continue;
        }

        wc_route* route = wc_get_route(wc, wc->request);
        wc_handle_route(wc, route);

        wc_http_write(wc);
    }
}
