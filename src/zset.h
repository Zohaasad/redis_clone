#pragma once

#include "skiplist.h"
#include "dict.h"
#include "sds.h"


struct ZSet {
    SkipList* sl;
    Dict*     ht; 

    ZSet();
    ~ZSet();
};


bool   zset_add(ZSet* z, const char* member, size_t mlen, double score);
bool   zset_rem(ZSet* z, const char* member, size_t mlen);
bool   zset_score(ZSet* z, const char* member, size_t mlen, double* score);
long   zset_rank(ZSet* z, const char* member, size_t mlen);
size_t zset_card(ZSet* z);


int    zset_range(ZSet* z, long start, long end,
                  char** members, double* scores, int max);
