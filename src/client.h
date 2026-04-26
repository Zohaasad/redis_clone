#pragma once

#include <string>
using namespace std;
struct Client {
    int         fd;
    string read_buf;
    string write_buf;
    bool        closing;

    Client(int fd);
    ~Client();
};