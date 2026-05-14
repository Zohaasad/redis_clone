#include "htable.h"
#include "object.h"
#include <cstring>

HTable::HTable()  { d = new Dict(); }
HTable::~HTable() { delete d; }


void htable_set(HTable* h, const char* field, size_t flen,
                            const char* value, size_t vlen) {
    sds  s   = sds_new(value, vlen);
    Obj* obj = new Obj(s);
    dict_set(h->d, field, flen, obj);
}


sds htable_get(HTable* h, const char* field, size_t flen) {
    Obj* obj = dict_get(h->d, field, flen);
    if (!obj) return nullptr;
    return obj->str;
}


bool htable_del(HTable* h, const char* field, size_t flen) {
    return dict_del(h->d, field, flen);
}


bool htable_exists(HTable* h, const char* field, size_t flen) {
    return dict_get(h->d, field, flen) != nullptr;
}


size_t htable_len(HTable* h) {
    return dict_size(h->d);
}

struct GetAllCtx {
    sds* keys;
    sds* vals;
    int  max;
    int  count;
};

static bool getall_cb(const char* key, size_t klen, Obj* val, void* ud) {
    GetAllCtx* ctx = (GetAllCtx*)ud;
    if (ctx->count >= ctx->max) return false;
    ctx->keys[ctx->count] = sds_new(key, klen);
    ctx->vals[ctx->count] = sds_dup(val->str);
    ctx->count++;
    return true;
}

int htable_getall(HTable* h, sds* keys, sds* vals, int max) {
    GetAllCtx ctx = { keys, vals, max, 0 };
    dict_each(h->d, getall_cb, &ctx);
    return ctx.count;
}
