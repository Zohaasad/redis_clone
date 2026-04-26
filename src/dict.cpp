#include "dict.h"
#include <cstdlib>
#include <cstring>
#define DICT_INITIAL_SIZE 16
#define DICT_LOAD_FACTOR  0.75

static size_t dict_hash(const char* key, size_t klen) {
    size_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < klen; i++) {
        hash ^= (unsigned char)key[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

Dict::Dict() {
    size    = DICT_INITIAL_SIZE;
    count   = 0;
    buckets = (DictEntry**)calloc(size, sizeof(DictEntry*));
}

Dict::~Dict() {
    for (size_t i = 0; i < size; i++) {
        DictEntry* e = buckets[i];
        while (e) {
            DictEntry* next = e->next;
            sds_free(e->key);
          
            delete e->val;
            free(e);
            e = next;
        }
    }
    free(buckets);
}


static void dict_resize(Dict* d, size_t new_size) {
    DictEntry** new_buckets = (DictEntry**)calloc(new_size, sizeof(DictEntry*));
    for (size_t i = 0; i < d->size; i++) {
        DictEntry* e = d->buckets[i];
        while (e) {
            DictEntry* next    = e->next;
            size_t     new_idx = dict_hash(e->key, sds_len(e->key)) & (new_size - 1);
            e->next            = new_buckets[new_idx];
            new_buckets[new_idx] = e;
            e = next;
        }
    }
    free(d->buckets);
    d->buckets = new_buckets;
    d->size    = new_size;
}
