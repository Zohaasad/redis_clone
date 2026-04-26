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