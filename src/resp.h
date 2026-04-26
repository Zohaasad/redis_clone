#pragma once

#include <string>
#include <vector>
using namespace std;


enum class RespStatus {
    OK,          
    INCOMPLETE,  
    ERROR        
};

struct RespResult {
    RespStatus           status;
    vector<string> args;    
    size_t               consumed;   
};


RespResult resp_parse(const char* buf, size_t buflen);

string resp_simple(const char* msg);
string resp_error(const char* msg);
string resp_integer(long long n);
string resp_bulk(const char* data, size_t len);
string resp_null_bulk();
string resp_array(const vector<string>& items);