#pragma once

#include <string>

struct Client {
    int         fd;
    std::string read_buf;
    std::string write_buf;
    bool        closing;

    Client(int fd);
    ~Client();
};