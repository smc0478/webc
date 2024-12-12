#ifndef _WEBC_H_
#define _WEBC_H_

#include "dict.h"
#include "net.h"
#include "response.h"
#include "request.h"

#define MAX_METHOD_LENGTH 10

struct _wc {
    wc_sock* server;
    dict routes[MAX_METHOD_LENGTH];
    wc_req* request;
    wc_resp* response;
};
typedef struct _wc wc;

typedef wc_resp* (*wc_handler)(wc_req);

struct _wc_route {
    char* method;
    char* path;
    wc_handler f;
};
typedef struct _wc_route wc_route;

extern wc wc_server;

wc* wc_alloc();
void wc_init(wc* wc, int port);
void wc_add_route(wc* wc, char* method, char* path, wc_handler f);
wc_route* wc_get_route(wc* wc, wc_req* req);
void wc_http_read(wc* wc);
void wc_http_write(wc* wc);
void wc_handle_route(wc* wc, wc_route* route);
void wc_server_init(int port);
void wc_server_add_route(char* method, char* path, wc_handler f);
void wc_server_start();
void wc_get(char* path, wc_handler f);
void wc_post(char* path, wc_handler f);

#endif // webc.h
