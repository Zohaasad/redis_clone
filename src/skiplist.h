#pragma once

#include <cstddef>
#include <cstring>

#define SKIPLIST_MAX_LEVEL 32
#define SKIPLIST_P         0.5


struct SkipListNode {
    char*          member;    
    double         score;
    int            level;      
    SkipListNode** forward;    

    SkipListNode(const char* m, size_t mlen, double s, int lvl);
    ~SkipListNode();
};


struct SkipList {
    SkipListNode* header;  
    int           level;    
    size_t        length;   

    SkipList();
    ~SkipList();
};

bool   sl_insert(SkipList* sl, const char* member, size_t mlen, double score);
bool   sl_delete(SkipList* sl, const char* member, size_t mlen);
bool   sl_get_score(SkipList* sl, const char* member, size_t mlen, double* score);
long   sl_get_rank(SkipList* sl, const char* member, size_t mlen);

int    sl_range_by_rank(SkipList* sl, long start, long end,
                        char** members, double* scores, int max);
size_t sl_length(SkipList* sl);
