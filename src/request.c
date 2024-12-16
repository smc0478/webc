
#include <stdlib.h>
#include <string.h>

#include "request.h"
#include "dict.h"

const char* method_str[] = {
#define V(str) #str,
    HTTP_METHOD(V)
#undef V
};



wc_req* wc_request_alloc() {
    wc_req* res = (wc_req*)malloc(sizeof(wc_req));
    wc_request_init(res);

    return res;
}

void wc_request_init(wc_req* req) {
    req->method = NULL;
    req->path   = NULL;
    dict_init(&req->params[QUERY]);
    dict_init(&req->params[BODY]);
    dict_init(&req->header);
    req->body = NULL;
    req->raw = NULL;
}

char* wc_request_get_method(wc_req *req) {
    return req->method;
}

char* wc_request_get_path(wc_req *req) {
    return req->path;
}

char* wc_request_get_param(wc_req *req, param kind, char *key) {
    char* value;

    if((value = (char*)dict_find_data(&req->params[kind], key)) == NULL)
        return NULL;

    return value;
}

char* wc_request_get_header(wc_req *req, char *key) {
    data* value;

    if((value = dict_find_data(&req->header, key)) == NULL)
        return NULL;

    return value->value;
}

char* wc_request_get_body(wc_req *req) {
    return req->body;
}

char* wc_request_get_raw(wc_req *req) {
    return req->raw;
}
