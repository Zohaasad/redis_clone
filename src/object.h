#pragma once
#include <cstdint>

typedef enum {
    OBJ_STRING = 0,
    OBJ_LIST   = 1,
    OBJ_HASH   = 2,
    OBJ_ZSET   = 3
} ObjType;
struct List;
struct HashTable;
struct SortedSet;

struct Obj {
    ObjType  type;
    int64_t  expire_at_ms;  

    union {
        char*       str;    
        List*       list;    
        HashTable*  hash;
        SortedSet*  zset;  
    };

    explicit Obj(char* s) : type(OBJ_STRING), expire_at_ms(0), str(s) {}
    Obj() : type(OBJ_STRING), expire_at_ms(0), str(nullptr) {}
};
