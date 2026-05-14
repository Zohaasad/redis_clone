#include "expiry.h"
#include <chrono>

int64_t now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()
    ).count();
}

bool check_expiry(Dict* d, const char* key, size_t klen) {
    Obj* obj = dict_get(d, key, klen);
    if (!obj) return false;
    if (obj->expire_at_ms == 0) return false;
    if (now_ms() < obj->expire_at_ms) return false;
    dict_del(d, key, klen);
    return true;
}