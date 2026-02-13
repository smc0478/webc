#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <strings.h>

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

static int wc_is_supported_http_version(const char* token, size_t len) {
    return (len == 8 && strncmp(token, "HTTP/1.1", 8) == 0) ||
           (len == 8 && strncmp(token, "HTTP/1.0", 8) == 0);
}

static int wc_is_token_char(char ch) {
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
           (ch >= '0' && ch <= '9') || ch == '-' || ch == '_';
}

static int wc_parse_size_t(const char* text, size_t len, size_t* out) {
    size_t value = 0;
    if (len == 0)
        return 0;

    for (size_t i = 0; i < len; i++) {
        if (!isdigit((unsigned char)text[i]))
            return 0;

        if (value > (SIZE_MAX - (size_t)(text[i] - '0')) / 10)
            return 0;

        value = value * 10 + (size_t)(text[i] - '0');
    }

    *out = value;
    return 1;
}

static int wc_validate_request(const char* http, size_t http_len) {
    if (http == NULL || http_len == 0 || http_len > WC_MAX_REQUEST_SIZE)
        return 0;

    size_t i = 0;
    size_t line_start = 0;

    while (i < http_len && http[i] != '\n')
        i++;
    if (i == http_len)
        return 0;

    size_t start_line_len = i - line_start;
    if (start_line_len > 0 && http[i - 1] == '\r')
        start_line_len--;
    if (start_line_len == 0 || start_line_len > WC_MAX_START_LINE_SIZE)
        return 0;

    size_t s1 = 0;
    while (s1 < start_line_len && http[s1] != ' ')
        s1++;
    if (s1 == 0 || s1 >= start_line_len)
        return 0;
    for (size_t k = 0; k < s1; k++)
        if (!wc_is_token_char(http[k]))
            return 0;

    size_t tstart = s1 + 1;
    size_t s2 = tstart;
    while (s2 < start_line_len && http[s2] != ' ')
        s2++;
    if (s2 == tstart || s2 >= start_line_len)
        return 0;
    if (http[tstart] != '/')
        return 0;

    size_t vstart = s2 + 1;
    size_t vlen = start_line_len - vstart;
    if (!wc_is_supported_http_version(http + vstart, vlen))
        return 0;

    i++;
    size_t header_count = 0;
    size_t content_length = 0;
    int has_content_length = 0;
    while (i < http_len) {
        line_start = i;
        while (i < http_len && http[i] != '\n')
            i++;
        if (i == http_len)
            return 0;

        size_t line_len = i - line_start;
        if (line_len > 0 && http[i - 1] == '\r')
            line_len--;

        i++;
        if (line_len == 0)
            break;
        if (line_len > WC_MAX_HEADER_LINE_SIZE)
            return 0;
        if (++header_count > WC_MAX_HEADER_COUNT)
            return 0;

        size_t colon = 0;
        while (colon < line_len && http[line_start + colon] != ':')
            colon++;
        if (colon == 0 || colon >= line_len)
            return 0;

        for (size_t k = 0; k < colon; k++) {
            char ch = http[line_start + k];
            if (!wc_is_token_char(ch))
                return 0;
        }

        size_t value_start = colon + 1;
        while (value_start < line_len &&
               (http[line_start + value_start] == ' ' || http[line_start + value_start] == '\t'))
            value_start++;
        size_t value_len = line_len - value_start;

        if (colon == strlen("Content-Length") &&
            strncasecmp(http + line_start, "Content-Length", colon) == 0) {
            size_t parsed = 0;
            if (!wc_parse_size_t(http + line_start + value_start, value_len, &parsed))
                return 0;
            has_content_length = 1;
            content_length = parsed;
        }
    }

    size_t body_len = http_len - i;
    if (has_content_length) {
        if (content_length != body_len)
            return 0;
        if (content_length > WC_MAX_BODY_SIZE)
            return 0;
    } else if (body_len > WC_MAX_BODY_SIZE) {
        return 0;
    }

    return 1;
}

wc_req* wc_parse_request(const char* http, size_t http_len) {
    if (!wc_validate_request(http, http_len))
        return NULL;

    char* request_text = wc_strndup_safe(http, http_len);
    if (request_text == NULL)
        return NULL;

    wc_req* req = wc_request_alloc();
    if (req == NULL) {
        free(request_text);
        return NULL;
    }

    req->raw = request_text;

    size_t token_start = 0;
    size_t body_start = http_len;
    char* pending_key = NULL;

    enum parse_state cur_state = req_start;

    for (size_t i = 0; i <= http_len; i++) {
        char ch = request_text[i];

        switch (cur_state) {
            case req_start:
                if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\0')
                    break;
                token_start = i;
                cur_state = req_method;
                break;

            case req_method:
                if (ch == ' ') {
                    req->method = wc_strndup_safe(request_text + token_start, i - token_start);
                    cur_state = req_method_fin;
                } else if (ch == '\0' || ch == '\r' || ch == '\n') {
                    req->method = wc_strndup_safe(request_text + token_start, i - token_start);
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
                    req->path = wc_strndup_safe(request_text + token_start, i - token_start);
                    token_start = i + 1;
                    cur_state = req_query_key;
                } else if (ch == ' ') {
                    req->path = wc_strndup_safe(request_text + token_start, i - token_start);
                    cur_state = req_proto_ver;
                } else if (ch == '\r' || ch == '\n' || ch == '\0') {
                    req->path = wc_strndup_safe(request_text + token_start, i - token_start);
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
                    pending_key = wc_strndup_safe(request_text + token_start, i - token_start);
                    token_start = i + 1;
                    cur_state = req_query_value;
                } else if (ch == '&') {
                    if (i > token_start)
                        wc_insert_kv(&req->params[QUERY], request_text + token_start, i - token_start, "", 0);
                    token_start = i + 1;
                } else if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\0') {
                    if (i > token_start)
                        wc_insert_kv(&req->params[QUERY], request_text + token_start, i - token_start, "", 0);

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
                        wc_insert_kv(&req->params[QUERY], pending_key, strlen(pending_key), request_text + token_start, i - token_start);
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
                    pending_key = wc_strndup_safe(request_text + token_start, i - token_start);
                    token_start = i + 1;
                    cur_state = req_header_value_start;
                } else if (ch == '\r' || ch == '\n' || ch == '\0') {
                    wc_insert_kv(&req->header, request_text + token_start, i - token_start, "", 0);

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
                        wc_insert_trimmed_header_value(req, pending_key, request_text + token_start, i - token_start);
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

    req->body = wc_strndup_safe(request_text + body_start, http_len - body_start);
    if (req->body != NULL && req->body[0] != '\0')
        wc_parse_key_value_pairs(&req->params[BODY], req->body, strlen(req->body));

    return req;
}
