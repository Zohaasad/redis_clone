#pragma once
#include <cstddef>
#include <cstring>

struct sds_header {
    size_t len;  
    size_t cap;   
    char   data[]; 
};
