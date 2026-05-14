#include "zset.h"
#include "object.h"
#include <cstdio>
#include <cstdlib>

ZSet::ZSet() {
    sl = new SkipList();
    ht = new Dict();
}

ZSet::~ZSet() {
    delete sl;
    delete ht;
}


static void ht_set_score(Dict* ht, const char* member, size_t mlen, double score) {
    char buf[32];
    int  len = snprintf(buf, sizeof(buf), "%.17g", score);
    sds  s   = sds_new(buf, len);
    Obj* obj = new Obj(s);
    dict_set(ht, member, mlen, obj);
}

static bool ht_get_score(Dict* ht, const char* member, size_t mlen, double* score) {
    Obj* obj = dict_get(ht, member, mlen);
    if (!obj) return false;
    *score = strtod(obj->str, nullptr);
    return true;
}


bool zset_add(ZSet* z, const char* member, size_t mlen, double score) {
    double old_score;
    if (ht_get_score(z->ht, member, mlen, &old_score)) {
       
        sl_delete(z->sl, member, mlen);
    }
    sl_insert(z->sl, member, mlen, score);
    ht_set_score(z->ht, member, mlen, score);
    return true;
}

bool zset_rem(ZSet* z, const char* member, size_t mlen) {
    if (!dict_get(z->ht, member, mlen)) return false;
    sl_delete(z->sl, member, mlen);
    dict_del(z->ht, member, mlen);
    return true;
}


bool zset_score(ZSet* z, const char* member, size_t mlen, double* score) {
    return ht_get_score(z->ht, member, mlen, score);
}


long zset_rank(ZSet* z, const char* member, size_t mlen) {
    return sl_get_rank(z->sl, member, mlen);
}


size_t zset_card(ZSet* z) {
    return sl_length(z->sl);
}


int zset_range(ZSet* z, long start, long end,
               char** members, double* scores, int max) {
    return sl_range_by_rank(z->sl, start, end, members, scores, max);
}
