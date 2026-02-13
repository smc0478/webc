#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <strings.h>

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


static int wc_find_header_end(const char* buf, size_t len, size_t* out_end) {
    if (len < 4)
        return 0;

    for (size_t i = 3; i < len; i++) {
        if (buf[i - 3] == '\r' && buf[i - 2] == '\n' && buf[i - 1] == '\r' && buf[i] == '\n') {
            *out_end = i + 1;
            return 1;
        }
    }

    return 0;
}

static int wc_parse_content_length(const char* headers, size_t headers_len, size_t* out_content_length) {
    size_t line_start = 0;
    *out_content_length = 0;

    while (line_start < headers_len) {
        size_t line_end = line_start;
        while (line_end < headers_len && headers[line_end] != '\n')
            line_end++;

        if (line_end == headers_len)
            return 0;

        size_t line_len = line_end - line_start;
        if (line_len > 0 && headers[line_end - 1] == '\r')
            line_len--;
        if (line_len == 0)
            return 1;

        size_t colon = 0;
        while (colon < line_len && headers[line_start + colon] != ':')
            colon++;

        if (colon == strlen("Content-Length") &&
            strncasecmp(headers + line_start, "Content-Length", colon) == 0) {
            size_t value_start = colon + 1;
            while (value_start < line_len && (headers[line_start + value_start] == ' ' || headers[line_start + value_start] == '\t'))
                value_start++;

            size_t value = 0;
            for (size_t i = value_start; i < line_len; i++) {
                char ch = headers[line_start + i];
                if (ch < '0' || ch > '9')
                    return 0;
                if (value > (WC_MAX_BODY_SIZE - (size_t)(ch - '0')) / 10)
                    return 0;
                value = (value * 10) + (size_t)(ch - '0');
            }

            *out_content_length = value;
            return 1;
        }

        line_start = line_end + 1;
    }

    return 1;
}

int wc_http_read(wc* wc) {
    wc_sock* sock = wc->server;
    char buf[WC_MAX_REQUEST_SIZE + 1];
    size_t total = 0;
    ssize_t nbytes;
    size_t header_end = 0;
    size_t content_length = 0;
    size_t expected_total = 0;
    int header_complete = 0;

    while (total < WC_MAX_REQUEST_SIZE) {
        nbytes = read(sock->client_fd, buf + total, WC_MAX_REQUEST_SIZE - total);
        if (nbytes <= 0)
            break;

        total += (size_t)nbytes;
        buf[total] = '\0';

        if (!header_complete && wc_find_header_end(buf, total, &header_end)) {
            header_complete = 1;
            if (!wc_parse_content_length(buf, header_end, &content_length))
                return -1;
            expected_total = header_end + content_length;

            if (expected_total > WC_MAX_REQUEST_SIZE)
                return -1;
        }

        if (header_complete && total >= expected_total)
            break;
    }

    if (total == 0 || total > WC_MAX_REQUEST_SIZE || !header_complete)
        return -1;

    if (expected_total > total)
        return -1;

    buf[expected_total] = '\0';
    wc->request = wc_parse_request(buf, expected_total);

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
