#ifndef _REQUEST_H_
#define _REQUEST_H_

#include <stddef.h>

#include "dict.h"

#define HTTP_METHOD(V)  \
    V(GET)              \
    V(HEAD)             \
    V(POST)             \
    V(PUT)              \
    V(DELETE)           \
    V(CONNECT)          \
    V(OPTIONS)          \
    V(TRACE)            \
    V(PATCH)            \
    V(OTHERS)           \


enum _methods {
#define V(str) str,
  HTTP_METHOD(V)
#undef V
};
typedef enum _methods methods;

extern const char* method_str[];

enum _param {
    QUERY,
    BODY
};
typedef enum _param param;

struct _wc_req {
    const char* method;
    char* path;
    dict params[2];
    dict header;
    char* body;
    const char* raw;
};
typedef struct _wc_req wc_req;

wc_req* wc_request_alloc();
void wc_request_init(wc_req* req);
char* wc_request_get_method(wc_req* req);
char* wc_request_get_path(wc_req* req);

char* wc_request_get_param(wc_req* req, param kind, char* key);

#define wc_request_get_query(req,key) \
    wc_request_get_param(req, QUERY, key);

#define wc_request_get_data(req,key) \
    wc_request_get_param(req, BODY, key);

char* wc_request_get_header(wc_req* req, char* key);

char* wc_request_get_body(wc_req* req);

char* wc_request_get_raw(wc_req* req);

#endif // request.h
