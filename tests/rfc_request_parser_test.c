#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/http.h"
#include "../src/dict.h"

static void free_dict_entries(dict* table) {
    if (table == NULL || table->bucket == NULL)
        return;

    for (int i = 0; i < table->capacity; i++) {
        if (table->bucket[i].state != USED)
            continue;

        free(table->bucket[i].key);
        if (table->bucket[i].free != NULL && table->bucket[i].value != NULL)
            table->bucket[i].free(table->bucket[i].value);
    }

    free(table->bucket);
    table->bucket = NULL;
    table->size = 0;
    table->capacity = 0;
}

static void wc_request_free_for_test(wc_req* req) {
    if (req == NULL)
        return;

    free((char*)req->method);
    free(req->path);
    free_dict_entries(&req->params[QUERY]);
    free_dict_entries(&req->params[BODY]);
    free_dict_entries(&req->header);
    free(req->body);
    free((char*)req->raw);
    free(req);
}

static int assert_not_null(const void* p, const char* message) {
    if (p != NULL)
        return 1;
    fprintf(stderr, "[FAIL] %s\n", message);
    return 0;
}

static int assert_null(const void* p, const char* message) {
    if (p == NULL)
        return 1;
    fprintf(stderr, "[FAIL] %s\n", message);
    return 0;
}

static int assert_str_eq(const char* actual, const char* expected, const char* message) {
    if (actual != NULL && strcmp(actual, expected) == 0)
        return 1;

    fprintf(stderr, "[FAIL] %s (expected='%s', actual='%s')\n",
            message,
            expected,
            actual == NULL ? "(null)" : actual);
    return 0;
}

/* RFC 7230/9112 style request-line + header-field grammar acceptance checks */
static int test_valid_minimal_get(void) {
    const char* raw = "GET /hello HTTP/1.1\r\nHost: localhost\r\n\r\n";
    wc_req* req = wc_parse_request(raw, strlen(raw));

    int ok = 1;
    ok &= assert_not_null(req, "valid minimal GET should parse");
    if (req != NULL) {
        ok &= assert_str_eq(req->method, "GET", "method should be GET");
        ok &= assert_str_eq(req->path, "/hello", "path should be /hello");
        ok &= assert_str_eq(wc_request_get_header(req, "Host"), "localhost", "Host header should parse");
    }

    wc_request_free_for_test(req);
    return ok;
}

static int test_valid_content_length_body(void) {
    const char* raw = "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 7\r\n\r\na=1&b=2";
    wc_req* req = wc_parse_request(raw, strlen(raw));

    int ok = 1;
    ok &= assert_not_null(req, "valid POST with Content-Length should parse");
    if (req != NULL) {
        ok &= assert_str_eq(wc_request_get_body(req), "a=1&b=2", "body should be preserved");
        ok &= assert_str_eq(wc_request_get_data(req, "a"), "1", "form value a should parse");
        ok &= assert_str_eq(wc_request_get_data(req, "b"), "2", "form value b should parse");
    }

    wc_request_free_for_test(req);
    return ok;
}

static int test_reject_invalid_http_version(void) {
    const char* raw = "GET / HTTP/2.0\r\nHost: localhost\r\n\r\n";
    wc_req* req = wc_parse_request(raw, strlen(raw));

    int ok = assert_null(req, "unsupported HTTP version should be rejected");
    wc_request_free_for_test(req);
    return ok;
}

static int test_reject_invalid_header_field_name(void) {
    const char* raw = "GET / HTTP/1.1\r\nBad Header: value\r\n\r\n";
    wc_req* req = wc_parse_request(raw, strlen(raw));

    int ok = assert_null(req, "header field-name with spaces should be rejected");
    wc_request_free_for_test(req);
    return ok;
}

static int test_reject_content_length_mismatch(void) {
    const char* raw = "POST /api HTTP/1.1\r\nHost: localhost\r\nContent-Length: 10\r\n\r\nabc";
    wc_req* req = wc_parse_request(raw, strlen(raw));

    int ok = assert_null(req, "Content-Length mismatch should be rejected");
    wc_request_free_for_test(req);
    return ok;
}

static int test_query_parameter_parsing(void) {
    const char* raw = "GET /find?name=webc&empty HTTP/1.1\r\nHost: localhost\r\n\r\n";
    wc_req* req = wc_parse_request(raw, strlen(raw));

    int ok = 1;
    ok &= assert_not_null(req, "query request should parse");
    if (req != NULL) {
        ok &= assert_str_eq(wc_request_get_query(req, "name"), "webc", "query value should parse");
        ok &= assert_str_eq(wc_request_get_query(req, "empty"), "", "query key without '=' should map to empty string");
    }

    wc_request_free_for_test(req);
    return ok;
}

int main(void) {
    int total = 0;
    int passed = 0;

    total++; passed += test_valid_minimal_get();
    total++; passed += test_valid_content_length_body();
    total++; passed += test_reject_invalid_http_version();
    total++; passed += test_reject_invalid_header_field_name();
    total++; passed += test_reject_content_length_mismatch();
    total++; passed += test_query_parameter_parsing();

    if (passed != total) {
        fprintf(stderr, "\nRFC parser tests: %d/%d passed\n", passed, total);
        return 1;
    }

    printf("RFC parser tests: %d/%d passed\n", passed, total);
    return 0;
}
