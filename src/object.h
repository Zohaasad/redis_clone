#pragma once

#include <cstdint>

typedef enum {
    OBJ_STRING = 0,
    OBJ_LIST   = 1,
    OBJ_HASH   = 2,
    OBJ_ZSET   = 3
} ObjType;


struct List;
struct HTable;
struct ZSet;

struct Obj {
    ObjType type;
    int64_t expire_at_ms;  

    union {
        char*   str;   
        List*   list;  
        HTable* hash;  
        ZSet*   zset;  
    };

    explicit Obj(char* s)  : type(OBJ_STRING), expire_at_ms(0), str(s)  {}
    explicit Obj(List* l)  : type(OBJ_LIST),   expire_at_ms(0), list(l) {}
    explicit Obj(HTable* h): type(OBJ_HASH),   expire_at_ms(0), hash(h) {}
    explicit Obj(ZSet* z)  : type(OBJ_ZSET),   expire_at_ms(0), zset(z) {}
    Obj() : type(OBJ_STRING), expire_at_ms(0), str(nullptr) {}
};