#pragma once
#include <cstddef>
#include <cstring>

struct sds_header {
    size_t len;  
    size_t cap;   
    char   data[]; 
};

typedef char* sds;

inline sds_header* sds_get_header(sds s) {
    return reinterpret_cast<sds_header*>(s - sizeof(sds_header));
}
