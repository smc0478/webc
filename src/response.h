#ifndef _RESPONSE_H_
#define _RESPONSE_H_

#include <stddef.h>

#include "dict.h"

struct _wc_resp {
    int status;
    dict headers;
    char* body;
};
typedef struct _wc_resp wc_resp;


wc_resp* wc_response_alloc();
void wc_response_init(wc_resp* resp);
void wc_response_set_status(wc_resp* resp, int status);
void wc_response_set_header(wc_resp* resp, char* key, char* value);
void wc_response_set_body(wc_resp* resp, char* body_format, ...);
wc_resp* wc_response(int status, char* body);
#endif // response.h
