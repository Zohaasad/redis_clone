#pragma once

#include "dict.h"
#include <cstdint>
#include <string>


#define RDB_MAGIC        "MRDB"
#define RDB_VERSION      1
#define RDB_TYPE_STRING  0
#define RDB_TYPE_LIST    1
#define RDB_TYPE_HASH    2
#define RDB_TYPE_ZSET    3


bool rdb_save(Dict* d, const std::string& path);

bool rdb_load(Dict* d, const std::string& path);
