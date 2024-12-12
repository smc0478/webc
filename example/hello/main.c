#include <stdio.h>
#include "webc.h"

wc_resp* hello(wc_req req) {
    return wc_response(200, "hello world!");
}

void registry_handler() {
    wc_get("/",hello);
}

int main() {
    wc_server_init(8080);

    registry_handler();

    wc_server_start();
}
