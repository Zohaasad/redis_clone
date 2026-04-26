#include "resp.h"
#include <cstring>
#include <cstdlib>
using namespace std;

static int find_crlf(const char* buf, size_t len) {
    for (size_t i = 0; i + 1 < len; i++) {
        if (buf[i] == '\r' && buf[i+1] == '\n') {
            return (int)i;
        }
    }
    return -1;
}


RespResult resp_parse(const char* buf, size_t buflen) {
    RespResult result;
    result.status   = RespStatus::INCOMPLETE;
    result.consumed = 0;

    if (buflen == 0) return result;
    if (buf[0] != '*') {
         int crlf = find_crlf(buf, buflen);
        if (crlf < 0) return result;  

        string line(buf, crlf);
        size_t pos = 0;
        while (pos < line.size()) {
            while (pos < line.size() && line[pos] == ' ') pos++;
            if (pos >= line.size()) break;
            size_t end = pos;
            while (end < line.size() && line[end] != ' ') end++;
            result.args.push_back(line.substr(pos, end - pos));
            pos = end;
        }

        if (result.args.empty()) {
            result.status   = RespStatus::ERROR;
            return result;
        }

        result.status   = RespStatus::OK;
        result.consumed = crlf + 2;   
        return result;
    }

    
    const char* p   = buf;
    size_t      rem = buflen;
    int crlf = find_crlf(p, rem);
    if (crlf < 0) return result;   

    if (p[0] != '*') {
        result.status = RespStatus::ERROR;
        return result;
    }

    long argc = strtol(p + 1, nullptr, 10);
    if (argc <= 0) {
        result.status = RespStatus::ERROR;
        return result;
    }

    p   += crlf + 2;
    rem -= crlf + 2;

    
    for (long i = 0; i < argc; i++) {
        if (rem < 3) return result;    

        if (p[0] != '$') {
            result.status = RespStatus::ERROR;
            return result;
        }

        int len_crlf = find_crlf(p, rem);
        if (len_crlf < 0) return result;    

        long slen = strtol(p + 1, nullptr, 10);
        if (slen < 0) {
            result.status = RespStatus::ERROR;
            return result;
        }

        p   += len_crlf + 2;
        rem -= len_crlf + 2;

         
        if (rem < (size_t)(slen + 2)) return result; 

        result.args.push_back(string(p, slen));

        p   += slen + 2;  
        rem -= slen + 2;
    }

    result.status   = RespStatus::OK;
    result.consumed = buflen - rem;
    return result;
}


string resp_simple(const char* msg) {
    return string("+") + msg + "\r\n";
}

string resp_error(const char* msg) {
    return string("-ERR ") + msg + "\r\n";
}

string resp_integer(long long n) {
    return string(":") + std::to_string(n) + "\r\n";
}

string resp_bulk(const char* data, size_t len) {
    string out = "$" + to_string(len) + "\r\n";
    out.append(data, len);
    out += "\r\n";
    return out;
}

string resp_null_bulk() {
    return "$-1\r\n";
}

string resp_array(const vector<string>& items) {
    string out = "*" + to_string(items.size()) + "\r\n";
    for (const auto& item : items) {
        out += resp_bulk(item.data(), item.size());
    }
    return out;
}