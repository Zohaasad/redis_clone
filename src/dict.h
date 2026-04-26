#pragma once

#include "sds.h"
#include "object.h"
#include <cstddef>


void   dict_set(Dict* d, const char* key, size_t klen, Obj* val);
Obj*   dict_get(Dict* d, const char* key, size_t klen);
bool   dict_del(Dict* d, const char* key, size_t klen);
size_t dict_size(Dict* d);

void   dict_each(Dict* d, bool (*fn)(const char* key, size_t klen, Obj* val, void* ud), void* ud);