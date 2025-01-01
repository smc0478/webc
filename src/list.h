#ifndef _LIST_H_
#define _LIST_H_

struct _list_head {
    struct _list_node* first;
};
typedef struct _list_head list_head;

struct _list_node {
    struct _list_node* next;
    void* ptr;
};
typedef struct _list_node list_node;

/**
 * list_new - allocation list
 *
 * allocation and initialize list
 */
list_head* list_new();

/**
 * list_init - initialize list
 * @head: the head of list to initialize
 */
void list_init(list_head* head);

/**
 * list_add - add new data in list
 * @head: the head of list to add data
 * @new: new data to be added
 */
void list_add(list_head* head, void* new);

/**
 * list_delete - delete data in list
 * @head: the head of list to delete data
 * @ptr: data to be deleted
 */
void list_delete(list_head* head, list_node* ptr);

/**
 * list_free - free all list
 * @head: the head of list to free
 */
void list_free(list_head* head);

/**
 * list_for_each - iterate all list
 * @pos: the &list_node to use as a loop cursor
 * @head: the head of list
 */
#define list_for_each(pos, head)    \
    for ((pos) = (head)->first; pos; (pos) = (pos)->next)

#endif // list.h
