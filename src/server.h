#pragma once

#include <unordered_map>

#ifdef __linux__
    #include <sys/epoll.h>
    #define USE_EPOLL 1
#else
    #include <sys/event.h>    
    #include <sys/time.h>
    #define USE_EPOLL 0
#endif

#include "client.h"
using namespace std;


#define MAX_EVENTS 128

struct Server {
    int  listen_fd;
    int  poll_fd;     
    int  port;

    unordered_map<int, Client*> clients;

    Server() : listen_fd(-1), poll_fd(-1), port(6380) {}
    ~Server();
};
bool server_init(Server* s, int port);   
void server_run(Server* s);             
void server_stop(Server* s);            