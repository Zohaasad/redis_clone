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
sds    sds_new(const char* init, size_t len); 
sds    sds_new_str(const char* init);       
sds    sds_empty();                         
size_t sds_len(sds s);                      
size_t sds_cap(sds s);          
sds    sds_append(sds s, const char* bytes, size_t len); 
sds    sds_append_str(sds s, const char* str);           
sds    sds_dup(sds s);                 
void   sds_free(sds s);                  
int    sds_cmp(sds a, sds b);               
