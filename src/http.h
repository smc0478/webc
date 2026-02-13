#ifndef _HTTP_H_
#define _HTTP_H_

#include "request.h"

#define WC_MAX_REQUEST_SIZE 8192
#define WC_MAX_START_LINE_SIZE 2048
#define WC_MAX_HEADER_LINE_SIZE 2048
#define WC_MAX_HEADER_COUNT 100
#define WC_MAX_BODY_SIZE 4096

enum parse_state {
    req_start,
    req_method,
    req_method_fin,
    req_path,
    req_query_key,
    req_query_value,
    req_proto_ver,
    req_start_line_almost_end,
    req_header_key_start,
    req_header_key,
    req_header_value_start,
    req_header_value,
    req_header_line_almost_end,
    req_header_almost_end,
    req_body_key,
    req_body_value,
    req_body_fin,
};

wc_req* wc_parse_request(const char* http, size_t http_len);


#endif // http.h
