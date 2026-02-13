#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "http.h"
#include "request.h"
#include "dict.h"

static char* wc_strndup_safe(const char* src, size_t len) {
    char* out = (char*)malloc(len + 1);
    if (out == NULL)
        return NULL;

    memcpy(out, src, len);
    out[len] = '\0';
    return out;
}

static void wc_insert_kv(dict* target, const char* key_start, size_t key_len,
                         const char* value_start, size_t value_len) {
    char* key = wc_strndup_safe(key_start, key_len);
    char* value = wc_strndup_safe(value_start, value_len);

    if (key == NULL || value == NULL) {
        free(key);
        free(value);
        return;
    }

    dict_insert(target, key, value, NULL);
    free(key);
}

static void wc_parse_key_value_pairs(dict* target, const char* text, size_t len) {
    size_t idx = 0;

    while (idx <= len) {
        size_t part_start = idx;
        size_t part_end = idx;

        while (part_end < len && text[part_end] != '&')
            part_end++;

        if (part_end > part_start) {
            size_t eq = part_start;
            while (eq < part_end && text[eq] != '=')
                eq++;

            if (eq < part_end)
                wc_insert_kv(target, text + part_start, eq - part_start,
                             text + eq + 1, part_end - (eq + 1));
            else
                wc_insert_kv(target, text + part_start, part_end - part_start, "", 0);
        }

        if (part_end == len)
            break;

        idx = part_end + 1;
    }
}

static void wc_insert_trimmed_header_value(wc_req* req, const char* key,
                                           const char* value_start, size_t value_len) {
    while (value_len > 0 && (value_start[value_len - 1] == ' ' || value_start[value_len - 1] == '\t'))
        value_len--;

    wc_insert_kv(&req->header, key, strlen(key), value_start, value_len);
}

wc_req* wc_parse_request(char* http) {
    if (http == NULL)
        return NULL;

    wc_req* req = wc_request_alloc();
    if (req == NULL)
        return NULL;

    size_t http_len = strlen(http);
    req->raw = strdup(http);

    size_t token_start = 0;
    size_t body_start = http_len;
    char* pending_key = NULL;

    enum parse_state cur_state = req_start;

    for (size_t i = 0; i <= http_len; i++) {
        char ch = (i < http_len) ? http[i] : '\0';

        switch (cur_state) {
            case req_start:
                if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\0')
                    break;
                token_start = i;
                cur_state = req_method;
                break;

            case req_method:
                if (ch == ' ') {
                    req->method = wc_strndup_safe(http + token_start, i - token_start);
                    cur_state = req_method_fin;
                } else if (ch == '\0' || ch == '\r' || ch == '\n') {
                    req->method = wc_strndup_safe(http + token_start, i - token_start);
                    cur_state = req_body_fin;
                }
                break;

            case req_method_fin:
                if (ch == ' ')
                    break;
                token_start = i;
                cur_state = req_path;
                i--;
                break;

            case req_path:
                if (ch == '?') {
                    req->path = wc_strndup_safe(http + token_start, i - token_start);
                    token_start = i + 1;
                    cur_state = req_query_key;
                } else if (ch == ' ') {
                    req->path = wc_strndup_safe(http + token_start, i - token_start);
                    cur_state = req_proto_ver;
                } else if (ch == '\r' || ch == '\n' || ch == '\0') {
                    req->path = wc_strndup_safe(http + token_start, i - token_start);
                    if (ch == '\n')
                        cur_state = req_header_key_start;
                    else if (ch == '\r')
                        cur_state = req_start_line_almost_end;
                    else
                        cur_state = req_body_fin;
                }
                break;

            case req_query_key:
                if (ch == '=') {
                    pending_key = wc_strndup_safe(http + token_start, i - token_start);
                    token_start = i + 1;
                    cur_state = req_query_value;
                } else if (ch == '&') {
                    if (i > token_start)
                        wc_insert_kv(&req->params[QUERY], http + token_start, i - token_start, "", 0);
                    token_start = i + 1;
                } else if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\0') {
                    if (i > token_start)
                        wc_insert_kv(&req->params[QUERY], http + token_start, i - token_start, "", 0);

                    if (ch == ' ')
                        cur_state = req_proto_ver;
                    else if (ch == '\n')
                        cur_state = req_header_key_start;
                    else if (ch == '\r')
                        cur_state = req_start_line_almost_end;
                    else
                        cur_state = req_body_fin;
                }
                break;

            case req_query_value:
                if (ch == '&' || ch == ' ' || ch == '\r' || ch == '\n' || ch == '\0') {
                    if (pending_key != NULL) {
                        wc_insert_kv(&req->params[QUERY], pending_key, strlen(pending_key), http + token_start, i - token_start);
                        free(pending_key);
                        pending_key = NULL;
                    }

                    if (ch == '&') {
                        token_start = i + 1;
                        cur_state = req_query_key;
                    } else if (ch == ' ') {
                        cur_state = req_proto_ver;
                    } else if (ch == '\n') {
                        cur_state = req_header_key_start;
                    } else if (ch == '\r') {
                        cur_state = req_start_line_almost_end;
                    } else {
                        cur_state = req_body_fin;
                    }
                }
                break;

            case req_proto_ver:
                if (ch == '\n')
                    cur_state = req_header_key_start;
                else if (ch == '\0')
                    cur_state = req_body_fin;
                break;

            case req_start_line_almost_end:
                if (ch == '\n')
                    cur_state = req_header_key_start;
                else if (ch == '\0')
                    cur_state = req_body_fin;
                break;

            case req_header_key_start:
                if (ch == '\r') {
                    cur_state = req_header_almost_end;
                } else if (ch == '\n') {
                    body_start = i + 1;
                    cur_state = req_body_key;
                } else if (ch == '\0') {
                    body_start = i;
                    cur_state = req_body_key;
                } else {
                    token_start = i;
                    cur_state = req_header_key;
                }
                break;

            case req_header_key:
                if (ch == ':') {
                    pending_key = wc_strndup_safe(http + token_start, i - token_start);
                    token_start = i + 1;
                    cur_state = req_header_value_start;
                } else if (ch == '\r' || ch == '\n' || ch == '\0') {
                    wc_insert_kv(&req->header, http + token_start, i - token_start, "", 0);

                    if (ch == '\r')
                        cur_state = req_header_line_almost_end;
                    else if (ch == '\n')
                        cur_state = req_header_key_start;
                    else
                        cur_state = req_body_fin;
                }
                break;

            case req_header_value_start:
                if (ch == ' ' || ch == '\t') {
                    token_start = i + 1;
                } else if (ch == '\r' || ch == '\n' || ch == '\0') {
                    if (pending_key != NULL) {
                        wc_insert_kv(&req->header, pending_key, strlen(pending_key), "", 0);
                        free(pending_key);
                        pending_key = NULL;
                    }

                    if (ch == '\r')
                        cur_state = req_header_line_almost_end;
                    else if (ch == '\n')
                        cur_state = req_header_key_start;
                    else
                        cur_state = req_body_fin;
                } else {
                    token_start = i;
                    cur_state = req_header_value;
                }
                break;

            case req_header_value:
                if (ch == '\r' || ch == '\n' || ch == '\0') {
                    if (pending_key != NULL) {
                        wc_insert_trimmed_header_value(req, pending_key, http + token_start, i - token_start);
                        free(pending_key);
                        pending_key = NULL;
                    }

                    if (ch == '\r')
                        cur_state = req_header_line_almost_end;
                    else if (ch == '\n')
                        cur_state = req_header_key_start;
                    else
                        cur_state = req_body_fin;
                }
                break;

            case req_header_line_almost_end:
                if (ch == '\n')
                    cur_state = req_header_key_start;
                else if (ch == '\r')
                    cur_state = req_header_almost_end;
                else if (ch != '\0') {
                    token_start = i;
                    cur_state = req_header_key;
                }
                else if (ch == '\0')
                    cur_state = req_body_fin;
                break;

            case req_header_almost_end:
                if (ch == '\n') {
                    body_start = i + 1;
                    cur_state = req_body_key;
                } else if (ch != '\r' && ch != '\0') {
                    token_start = i;
                    cur_state = req_header_key;
                } else if (ch == '\0') {
                    body_start = i;
                    cur_state = req_body_key;
                }
                break;

            case req_body_key:
                if (body_start > http_len)
                    body_start = i;
                cur_state = req_body_fin;
                break;

            case req_body_value:
            case req_body_fin:
                break;
        }
    }

    if (pending_key != NULL)
        free(pending_key);

    if (body_start > http_len)
        body_start = http_len;

    req->body = wc_strndup_safe(http + body_start, http_len - body_start);
    if (req->body != NULL && req->body[0] != '\0')
        wc_parse_key_value_pairs(&req->params[BODY], req->body, strlen(req->body));

    return req;
}
