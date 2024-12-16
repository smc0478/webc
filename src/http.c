#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "http.h"
#include "request.h"
#include "dict.h"





// TODO(Exception handling, optimization,
//    and proper parsing)
wc_req* wc_parse_request(char *http) {
    wc_req* req = wc_request_alloc();

    req->raw = strdup(http);
    size_t http_len = strlen(http);
    size_t prev_idx = 0;
    size_t body_start = 0;

    enum parse_state cur_state = req_start;
    methods cur_method;
    char* match;
    char* key, *value;

    for(size_t i = 0; i < http_len; i++) {
        char ch = http[i];

        switch(cur_state) {
            case req_start:
            {
                switch(ch) {
                    case 'C':
                        cur_method = CONNECT;
                        break;
                    case 'D':
                        cur_method = DELETE;
                        break;
                    case 'G':
                        cur_method = GET;
                        break;
                    case 'H':
                        cur_method = HEAD;
                        break;
                    case 'O':
                        cur_method = OPTIONS;
                        break;
                    case 'P':
                        cur_method = POST;
                        break;
                    case 'T':
                        cur_method = TRACE;
                        break;
                    default:
                        cur_method = OTHERS;
                        break;
                }

                cur_state = req_method;
                break;
            }
            case req_method:
            {
                match = method_str[cur_method];
                if(ch == ' ') {
                    cur_state = req_method_fin;
                    continue;
                } else if(cur_method == OTHERS ||
                   ch == match[i-prev_idx])
                    continue;

                switch((cur_method<<8)|(ch)) {
                    case ((POST<<8)|('A')):
                        cur_method = PATCH;
                        break;
                    case ((POST<<8)|('U')):
                        cur_method = PUT;
                        break;
                    default:
                        cur_method = OTHERS;
                        break;
                }
                break;
            }
            case req_method_fin:
            {
                if(cur_method == OTHERS || match[i-prev_idx-1] != '\0')
                    req->method = strndup(&http[prev_idx],i-prev_idx-1);
                else
                    req->method = method_str[cur_method];

                prev_idx = i;

                cur_state = req_path;
                if(ch != '/')
                    i--;

                break;
            }
            case req_path:
            {
                if(ch == ' ') {
                    req->path = strndup(&http[prev_idx],i-prev_idx);
                    cur_state = req_proto_ver;
                } else if(ch == '?') {
                    req->path = strndup(&http[prev_idx],i-prev_idx);
                    prev_idx = i + 1;
                    cur_state = req_query_key;
                }
                break;
            }
            case req_query_key:
            {
                if(ch == ' '&&i!=prev_idx) {
                    key = strndup(&http[prev_idx], i-prev_idx);
                    value = strdup("");
                    dict_insert(&req->params[QUERY], key, value, NULL);

                    free(key);

                    cur_state = req_proto_ver;
                } else if(ch == '=') {
                    key = strndup(&http[prev_idx], i-prev_idx);

                    prev_idx = i + 1;
                    cur_state = req_query_value;
                } else if(ch == ' ')
                    cur_state = req_proto_ver;

                break;
            }
            case req_query_value:
            {
                if(ch == ' ') {
                    value = strndup(&http[prev_idx], i-prev_idx);
                    dict_insert(&req->params[QUERY], key, value, NULL);
                    free(key);
                    cur_state = req_proto_ver;
                } else if(ch == '&') {
                    value = strndup(&http[prev_idx], i-prev_idx);
                    dict_insert(&req->params[QUERY], key, value, NULL);
                    free(key);

                    prev_idx = i + 1;

                    cur_state = req_query_key;
                }
                break;
            }
            case req_proto_ver:
            {
                if(ch == '\r')
                    cur_state = req_start_line_almost_end;
                else if(ch == '\n')
                    cur_state = req_header_key_start;

                break;
            }
            case req_start_line_almost_end:
            {
                if(ch == '\n')
                    cur_state = req_header_key_start;

                break;
            }
            case req_header_key_start:
            {
                if(ch == '\r') {
                    cur_state = req_header_almost_end;
                } else if(ch == '\n') {
                    cur_state = req_body_key;
                    body_start = i + 1;
                    prev_idx = i + 1;
                } else if (ch == ':') {
                    key = strdup("");
                    prev_idx = i + 1;
                    cur_state = req_header_value_start;
                } else {
                    prev_idx = i;
                    cur_state = req_header_key;
                }
                break;
            }
            case req_header_key:
            {
                if(ch == ':') {
                    key = strndup(&http[prev_idx], i-prev_idx);
                    cur_state = req_header_value_start;
                    prev_idx = i + 1;
                } else if(ch == '\r') {
                    key = strndup(&http[prev_idx], i-prev_idx);
                    value = strdup("");
                    dict_insert(&req->header, key, value, NULL);
                    free(key);
                    cur_state = req_header_line_almost_end;
                } else if(ch == '\n') {
                    key = strndup(&http[prev_idx], i-prev_idx);
                    value = strdup("");
                    dict_insert(&req->header, key, value, NULL);
                    free(key);
                    cur_state = req_header_key_start;
                }

                break;
            }
            case req_header_value_start:
            {
                if(ch == '\r') {
                    value = strdup("");
                    dict_insert(&req->header, key, value, NULL);
                    free(key);
                    cur_state = req_header_line_almost_end;
                } else if(ch == '\n') {
                    value = strdup("");
                    dict_insert(&req->header, key, value, NULL);
                    free(key);
                    cur_state = req_header_key_start;
                } else if(ch == ' ')
                    prev_idx = i + 1;
                else
                    cur_state = req_header_value;
                break;
            }
            case req_header_value:
            {
                if(ch == '\r') {
                    value = strndup(&http[prev_idx], i-prev_idx);
                    dict_insert(&req->header, key, value, NULL);
                    free(key);
                    cur_state = req_header_line_almost_end;
                } else if(ch == '\n') {
                    value = strndup(&http[prev_idx], i-prev_idx);
                    dict_insert(&req->header, key, value, NULL);
                    free(key);
                    cur_state = req_header_key_start;
                }
                break;
            }
            case req_header_line_almost_end:
            {
                if(ch == '\n')
                    cur_state = req_header_key_start;

                break;
            }
            case req_header_almost_end:
            {
                if(ch == '\n') {
                    cur_state = req_body_key;
                    prev_idx = i + 1;
                    body_start = i + 1;
                }

                break;
            }
            case req_body_key:
            {
                if(ch == ':') {
                    key = strndup(&http[prev_idx], i-prev_idx);
                    prev_idx = i + 1;
                    cur_state = req_body_value;
                }
                break;
            }
            case req_body_value:
            {
                if(ch == '&') {
                    value = strndup(&http[prev_idx], i-prev_idx);
                    dict_insert(&req->params[BODY], key, value, NULL);
                    free(key);
                    prev_idx = i + 1;
                    cur_state = req_body_key;
                }
                break;
            }
        }
    }
    switch(cur_state) {
        case req_header_key:
            key = strndup(&http[prev_idx], http_len-prev_idx);
        case req_header_value_start:
        {
            value = strdup("");
            dict_insert(&req->header, key, value, NULL);
            free(key);
            break;
        }
        case req_header_value:
        {
            value = strndup(&http[prev_idx], http_len-prev_idx);
            dict_insert(&req->header, key, value, NULL);
            free(key);
        }
        case req_body_key:
        {
            if(prev_idx >= http_len)
                break;
            key = strndup(&http[prev_idx], http_len-prev_idx);
            value = strdup("");
            dict_insert(&req->params[BODY], key, value, NULL);
            free(key);
            break;
        }
        case req_body_value:
        {
            if(prev_idx >= http_len)
                value = strdup("");
            else
                value = strndup(&http[prev_idx], http_len-prev_idx);

            dict_insert(&req->params[BODY], key, value, NULL);
            free(key);
            break;
        }
        default:
            break;
    }
    req->body = strndup(&http[body_start], http_len-body_start);


    return req;
}
