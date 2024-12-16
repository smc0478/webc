#include <stdio.h>
#include "webc.h"

wc_resp* hello(wc_req* req) {
    return wc_response(200, "<p>hello world!</p>");
}

wc_resp* hello_name(wc_req* req) {
    char* name = wc_request_get_query(req, "name");
    wc_resp* ret = wc_response_alloc();

    wc_response_set_status(ret, 200);
    wc_response_set_body(ret, "<p>hello %s!</p>", name);

    return ret;
}
void registry_handler() {
    wc_get("/",hello);
    wc_get("/name",hello_name);
}

int main() {
    wc_server_init(8080);

    registry_handler();

    wc_server_start();
}
