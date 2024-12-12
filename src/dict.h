#ifndef _DICT_H_
#define _DICT_H_

typedef void (*free_fp)(void*);

enum state {
    EMPTY,
    USED,
    FREE,
};

struct _data {
    char* key;
    void* value;
    enum state state;
    unsigned long hash_value;
    free_fp free;
};

typedef struct _data data;

struct _dict {
    data* bucket;
    int size;
    int capacity;
};
typedef struct _dict dict;

dict* dict_alloc();
void dict_init(dict* dict);
void dict_insert(dict *dict, char *key, void *value, free_fp free_fp);
void dict_delete(dict *dict, char *key);
void* dict_find_data(dict *dict, char *key);
void data_free(data* data);
#endif // dict.h
