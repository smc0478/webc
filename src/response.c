#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "response.h"
#include "dict.h"

wc_resp* wc_response_alloc() {
    wc_resp* res = (wc_resp*)malloc(sizeof(wc_resp));
    wc_response_init(res);

    return res;
}

void wc_response_init(wc_resp* resp) {
    resp->status = 200;
    dict_init(&resp->headers);

    dict_insert(&resp->headers, "Server", "WEBC/0.0.1", (free_fp)-1);

    resp->body = NULL;
}

void wc_response_set_status(wc_resp* resp, int status) {
    resp->status = status;
}

void wc_response_set_header(wc_resp* resp, char* key, char* value) {

    dict_insert(&resp->headers, key, strdup(value), NULL);
}

static void wc_response_set_length(wc_resp* resp, size_t len) {

    char tmp[16];

    sprintf(tmp, "%lu", len);

    wc_response_set_header(resp, "Content-Length", tmp);
}

void wc_response_set_body(wc_resp* resp, char* body_format, ...) {
    char buf[1024*2];
    va_list arg_ptr;

    va_start(arg_ptr, body_format);

    vsnprintf(buf, sizeof(buf), body_format, arg_ptr);

    resp->body = strdup(buf);

    wc_response_set_length(resp, strlen(buf));

    va_end(arg_ptr);
}

wc_resp* wc_response(int status, char *body) {
    wc_resp* ret = wc_response_alloc();

    wc_response_set_status(ret, status);
    wc_response_set_body(ret, body);

    return ret;
}
