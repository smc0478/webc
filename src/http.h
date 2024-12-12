#ifndef _HTTP_H_
#define _HTTP_H_

#include "request.h"

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

wc_req* wc_parse_request(char* http);


#endif // http.h
