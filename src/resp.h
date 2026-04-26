#pragma once

#include <string>
#include <vector>


enum class RespStatus {
    OK,          
    INCOMPLETE,  
    ERROR        
};

struct RespResult {
    RespStatus           status;
    std::vector<std::string> args;    
    size_t               consumed;   
};


RespResult resp_parse(const char* buf, size_t buflen);

std::string resp_simple(const char* msg);
std::string resp_error(const char* msg);
std::string resp_integer(long long n);
std::string resp_bulk(const char* data, size_t len);
std::string resp_null_bulk();
std::string resp_array(const std::vector<std::string>& items);