#pragma once
#include <cstdint>

typedef enum {
    OBJ_STRING = 0,
    OBJ_LIST   = 1,
    OBJ_HASH   = 2,
    OBJ_ZSET   = 3
} ObjType;