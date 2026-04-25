#include "sds.h"
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <new>  
using namespace std;

static sds sds_alloc(size_t cap) {
    sds_header* h = (sds_header*)malloc(sizeof(sds_header) + cap + 1);
    if (!h) throw bad_alloc();
    h->len = 0;
    h->cap = cap;
    h->data[0] = '\0';
    return h->data;
}

sds sds_new(const char* init, size_t len) {
    sds s = sds_alloc(len);
    if (init && len > 0) {
        memcpy(s, init, len);
    }
    sds_get_header(s)->len = len;
    s[len] = '\0';
    return s;
}