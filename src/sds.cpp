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
sds sds_new_str(const char* init) {
    size_t len = init ? strlen(init) : 0;
    return sds_new(init, len);
}

sds sds_empty() {
    return sds_new(nullptr, 0);
}
size_t sds_len(sds s) {
    return sds_get_header(s)->len;
}

size_t sds_cap(sds s) {
    return sds_get_header(s)->cap;
}

sds sds_append(sds s, const char* bytes, size_t len) {
    sds_header* h   = sds_get_header(s);
    size_t      cur = h->len;
    size_t      needed = cur + len;

    if (needed > h->cap) {
        size_t new_cap = h->cap * 2;
        if (new_cap < needed) new_cap = needed;

        h = (sds_header*)realloc(h, sizeof(sds_header) + new_cap + 1);
        if (!h) throw bad_alloc();
        h->cap = new_cap;
        s = h->data;   
    }

    memcpy(s + cur, bytes, len);
    h->len       = needed;
    s[needed]    = '\0';
    return s;
}

sds sds_append_str(sds s, const char* str) {
    return sds_append(s, str, strlen(str));
}

sds sds_dup(sds s) {
    return sds_new(s, sds_len(s));
}

void sds_free(sds s) {
    if (s) free(sds_get_header(s));
}
int sds_cmp(sds a, sds b) {
    size_t la = sds_len(a);
    size_t lb = sds_len(b);
    size_t min_len = la < lb ? la : lb;
    int r = memcmp(a, b, min_len);
    if (r != 0) return r;
    if (la < lb) return -1;
    if (la > lb) return  1;
    return 0;
}