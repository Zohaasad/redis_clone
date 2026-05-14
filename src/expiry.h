#pragma once

#include "object.h"
#include "dict.h"
#include <cstdint>

int64_t now_ms();
bool check_expiry(Dict* d, const char* key, size_t klen);