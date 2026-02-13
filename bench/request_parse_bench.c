#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
}

static void wc_request_free_for_bench(wc_req* req) {
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

static double diff_ms(const struct timespec* start, const struct timespec* end) {
    double sec = (double)(end->tv_sec - start->tv_sec) * 1000.0;
    double nsec = (double)(end->tv_nsec - start->tv_nsec) / 1000000.0;
    return sec + nsec;
}

int main(int argc, char** argv) {
    const char* raw =
        "POST /bench?env=dev HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "User-Agent: webc-bench\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 24\r\n"
        "\r\n"
        "name=webc&mode=benchmark";

    int iterations = 100000;
    if (argc >= 2) {
        iterations = atoi(argv[1]);
        if (iterations <= 0)
            iterations = 100000;
    }

    struct timespec start;
    struct timespec end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < iterations; i++) {
        wc_req* req = wc_parse_request(raw, strlen(raw));
        if (req == NULL) {
            fprintf(stderr, "parse failure at iteration %d\n", i);
            return 1;
        }
        wc_request_free_for_bench(req);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed_ms = diff_ms(&start, &end);
    double req_per_sec = ((double)iterations / elapsed_ms) * 1000.0;
    printf("Parsed %d requests in %.3f ms (%.2f req/s)\n", iterations, elapsed_ms, req_per_sec);

    return 0;
}
