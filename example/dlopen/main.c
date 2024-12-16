#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <webc.h>

void* handle;


void dlsym_error() {
    char* err;
    if((err = dlerror()) != NULL) {
        fprintf(stderr, "dlsym error: %s\n", err);
        exit(-1);
    }
}

wc_resp* hello(wc_req* req) {
    wc_resp* (*make_resp)(int, char*);
    make_resp = dlsym(handle, "wc_response");
    dlsym_error();
    return make_resp(200, "hello world!");
}


int main() {
    handle = dlopen("libwebc.so",RTLD_LAZY);
    char* err;
    if (!handle) {
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        return -1;
    }
    void (*server_init)(int);
    void (*get)(char*,wc_handler);
    void (*server_start)();

    server_init = dlsym(handle, "wc_server_init");
    dlsym_error();
    get = dlsym(handle, "wc_get");
    dlsym_error();
    server_start = dlsym(handle, "wc_server_start");

    server_init(8080);

    get("/",hello);

    server_start();

    dlclose(handle);
}
