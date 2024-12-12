#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "dict.h"

// http://www.cse.yorku.ca/~oz/hash.html
static unsigned long hash(unsigned char *str) {
  unsigned long hash = 5381;
  int c;

  while (c = *str++)
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}

static int _dict_find_idx(dict *dict, char *key) {
  unsigned long hash_value = hash((unsigned char *)key);
  unsigned long idx = hash_value % dict->capacity;

  for (int i = 0; i < dict->capacity; i++) {
    if (dict->bucket[idx].state == EMPTY)
      break;
    else if (dict->bucket[idx].state == FREE)
      continue;
    else if (strcmp(dict->bucket[idx].key, key) == 0)
      return idx;

    idx = (idx + 1) % dict->capacity;
  }

  return -1;
}

static int get_next_prime(int num) {
  while (true) {
    bool is_prime = true;
    for (int i = 0; i * i <= num; i++) {
      if (num % i == 0) {
        is_prime = false;
        break;
      }
    }

    if (is_prime)
      return num;

    num++;
  }
}

static void _dict_realloc_data(dict *dict, int size) {
  int prev_capacity = dict->capacity;

  if (size == 0)
    dict->capacity = 7; // initial capacity
  else
    dict->capacity = get_next_prime(size);

  data *new_bucket = (data *)malloc(sizeof(data) * dict->capacity);
  for (int i = 0; i < dict->capacity; ++i)
    new_bucket[i].state = EMPTY;

  for (int i = 0; i < prev_capacity; i++) {
    if (dict->bucket[i].state != USED)
      continue;

    unsigned long idx = dict->bucket[i].hash_value;
    idx %= dict->capacity;

    while (new_bucket[idx].state == USED) {
      if (strcmp(new_bucket[idx].key, dict->bucket[i].key) == 0)
        break;

      idx++;
    }

    memcpy(&new_bucket[idx], &dict->bucket[i], sizeof(data));
  }
  free(dict->bucket);
  dict->bucket = new_bucket;
}

dict* dict_alloc() {
  dict *ret = (dict *)malloc(sizeof(dict));
  dict_init(ret);
  return ret;
}

void dict_init(dict *dict) {
  if (dict->bucket)
    free(dict->bucket);
  dict->bucket = NULL;
  dict->size = 0;
  dict->capacity = 0;
}

void dict_insert(dict *dict, char *key, void *value, free_fp free_f) {
  if (dict->size * 3 >= dict->capacity * 2)
      _dict_realloc_data(dict, dict->capacity);

  unsigned long hash_value = hash((unsigned char *)key);
  unsigned long idx = hash_value % dict->capacity;

  while (dict->bucket[idx].state == USED) {
    if (strcmp(dict->bucket[idx].key, key) == 0) {
      if (dict->bucket[idx].free != NULL)
        dict->bucket[idx].free(dict->bucket[idx].value);
      break;
    }
    idx = (idx + 1) % dict->capacity;
  }

  if (dict->bucket[idx].state != USED) {
    dict->bucket[idx].key = strdup(key);
    dict->bucket[idx].state = USED;
  }
  dict->bucket[idx].hash_value = hash_value;
  dict->bucket[idx].value = value;
  if(free_f == NULL)
    dict->bucket[idx].free = free;
  else if(free_f == (free_fp)-1)
    dict->bucket[idx].free = NULL;
  else
    dict->bucket[idx].free = free_f;
  dict->size++;
}

void dict_delete(dict *dict, char *key) {
  int idx = _dict_find_idx(dict, key);
  if (idx == -1)
    return;

  free(dict->bucket[idx].key);
  if(dict->bucket[idx].free)
      dict->bucket[idx].free(dict->bucket[idx].value);
  dict->bucket[idx].state = FREE;
  dict->size--;
}

void* dict_find_data(dict *dict, char *key) {
    int idx = _dict_find_idx(dict, key);
    if (idx == -1)
        return NULL;

    return dict->bucket[idx].value;
}

void dict_clear(dict *dict) {
    for (int i = 0; i < dict->capacity; i++) {
        if (dict->bucket[i].state == USED)
            free(dict->bucket[i].key);
    }

    dict_init(dict);
}
