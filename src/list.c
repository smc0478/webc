#include <stdlib.h>

#include "list.h"


list_head* list_new() {
    list_head* ret = (list_head*)malloc(sizeof(list_head));
    list_init(ret);
    return ret;
}

void list_init(list_head* head) {
    head->first = NULL;
}

void list_add(list_head* head, void* new) {
    list_node* cur = (list_node*)head;
    while(cur->next)  cur = cur->next;

    cur->next = (list_node*)malloc(sizeof(list_node));
    cur->next->ptr = new;
}

void list_delete(list_head* head, list_node* ptr) {
    list_node* cur = (list_node*)head;
    list_node* next;
    while(cur->next == ptr)     cur = cur->next;

    next = cur->next;
    cur->next = next->next;
    free(next);
}

void list_free(list_head* head) {
    free(head);
}
