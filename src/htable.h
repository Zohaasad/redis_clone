#pragma once

#include "dict.h"
#include "sds.h"


struct HTable {
    Dict* d;

    HTable();
    ~HTable();
};


void   htable_set(HTable* h, const char* field, size_t flen,
                              const char* value, size_t vlen);
sds    htable_get(HTable* h, const char* field, size_t flen);
bool   htable_del(HTable* h, const char* field, size_t flen);
bool   htable_exists(HTable* h, const char* field, size_t flen);
size_t htable_len(HTable* h);


int    htable_getall(HTable* h, sds* keys, sds* vals, int max);
