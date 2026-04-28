#include "server.h"
#include "client.h"
#include "resp.h"
#include "commands.h"
using namespace std;
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cerrno>

static bool set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}



#if USE_EPOLL

static bool event_add_read(Server* s, int fd) {
    epoll_event ev{};
    ev.events  = EPOLLIN;
    ev.data.fd = fd;
    return epoll_ctl(s->poll_fd, EPOLL_CTL_ADD, fd, &ev) == 0;
}

static bool event_set_write(Server* s, int fd, bool want_write) {
    epoll_event ev{};
    ev.events  = EPOLLIN | (want_write ? EPOLLOUT : 0);
    ev.data.fd = fd;
    return epoll_ctl(s->poll_fd, EPOLL_CTL_MOD, fd, &ev) == 0;
}

static bool event_del(Server* s, int fd) {
    return epoll_ctl(s->poll_fd, EPOLL_CTL_DEL, fd, nullptr) == 0;
}

#else  

static bool event_add_read(Server* s, int fd) {
    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
    return kevent(s->poll_fd, &ev, 1, nullptr, 0, nullptr) == 0;
}

static bool event_set_write(Server* s, int fd, bool want_write) {
    struct kevent ev;
    if (want_write) {
        EV_SET(&ev, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, nullptr);
    } else {
        EV_SET(&ev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    }
    kevent(s->poll_fd, &ev, 1, nullptr, 0, nullptr);
    return true;
}

static bool event_del(Server* s, int fd) {
    struct kevent ev[2];
    EV_SET(&ev[0], fd, EVFILT_READ,  EV_DELETE, 0, 0, nullptr);
    EV_SET(&ev[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    kevent(s->poll_fd, ev, 2, nullptr, 0, nullptr);
    return true;
}

#endif


struct Event {
    int  fd;
    bool readable;
    bool writable;
};

static int event_wait(Server* s, Event* events, int max) {
#if USE_EPOLL
    epoll_event raw[MAX_EVENTS];
    int n = epoll_wait(s->poll_fd, raw, max, -1);
    if (n < 0) return 0;
    for (int i = 0; i < n; i++) {
        events[i].fd       = raw[i].data.fd;
        events[i].readable = raw[i].events & EPOLLIN;
        events[i].writable = raw[i].events & EPOLLOUT;
    }
    return n;
#else
    struct kevent raw[MAX_EVENTS];
    int n = kevent(s->poll_fd, nullptr, 0, raw, max, nullptr);
    if (n < 0) return 0;
    for (int i = 0; i < n; i++) {
        events[i].fd       = (int)raw[i].ident;
        events[i].readable = raw[i].filter == EVFILT_READ;
        events[i].writable = raw[i].filter == EVFILT_WRITE;
    }
    return n;
#endif
}
static void client_close(Server* s, int fd) {
    event_del(s, fd);
    auto it = s->clients.find(fd);
    if (it != s->clients.end()) {
        delete it->second;
        s->clients.erase(it);
    }
}


static void handle_accept(Server* s) {
    sockaddr_in addr{};
    socklen_t   addrlen = sizeof(addr);
    int cfd = accept(s->listen_fd, (sockaddr*)&addr, &addrlen);
    if (cfd < 0) return;

    set_nonblocking(cfd);
    event_add_read(s, cfd);

    Client* c = new Client(cfd);
    s->clients[cfd] = c;

    printf("[minired] client connected fd=%d\n", cfd);
}

static void handle_read(Server* s, int fd) {
    Client* c = s->clients[fd];
    char tmp[4096];

    while (true) {
        ssize_t n = recv(fd, tmp, sizeof(tmp), 0);
        if (n > 0) {
            c->read_buf.append(tmp, n);
        } else if (n == 0) {
            
            client_close(s, fd);
            return;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            client_close(s, fd);
            return;
        }
    }

    
    while (!c->read_buf.empty()) {
        RespResult res = resp_parse(c->read_buf.data(), c->read_buf.size());

        if (res.status == RespStatus::INCOMPLETE) break;

        if (res.status == RespStatus::ERROR) {
            c->write_buf += "-ERR protocol error\r\n";
            client_close(s, fd);
            return;
        }

       
        dispatch_command(c, res.args);

        
        c->read_buf.erase(0, res.consumed);

        if (c->closing) {
            client_close(s, fd);
            return;
        }
    }

    
    if (!c->write_buf.empty()) {
        event_set_write(s, fd, true);
    }
}


static void handle_write(Server* s, int fd) {
    Client* c = s->clients[fd];

    while (!c->write_buf.empty()) {
        ssize_t n = send(fd, c->write_buf.data(), c->write_buf.size(), 0);
        if (n > 0) {
            c->write_buf.erase(0, n);
        } else if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            client_close(s, fd);
            return;
        }
    }

    
    if (c->write_buf.empty()) {
        event_set_write(s, fd, false);
    }
}


bool server_init(Server* s, int port) {
    s->port = port;

   
    s->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->listen_fd < 0) {
        perror("socket");
        return false;
    }

    
    int opt = 1;
    setsockopt(s->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    set_nonblocking(s->listen_fd);

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(s->listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return false;
    }

    if (listen(s->listen_fd, 128) < 0) {
        perror("listen");
        return false;
    }

   
#if USE_EPOLL
    s->poll_fd = epoll_create1(0);
#else
    s->poll_fd = kqueue();
#endif

    if (s->poll_fd < 0) {
        perror("epoll/kqueue create");
        return false;
    }

   
    event_add_read(s, s->listen_fd);

    printf("[minired] starting on port %d\n", port);
    return true;
}

void server_run(Server* s) {
    printf("[minired] event loop ready\n");

    Event events[MAX_EVENTS];

    while (true) {
        int n = event_wait(s, events, MAX_EVENTS);

        for (int i = 0; i < n; i++) {
            int fd = events[i].fd;

            if (fd == s->listen_fd) {
               
                handle_accept(s);
            } else {
               
                if (events[i].readable) handle_read(s, fd);
                if (s->clients.count(fd) && events[i].writable) handle_write(s, fd);
            }
        }
    }
}
void server_stop(Server* s) {
    for (auto& [fd, client] : s->clients) {
        delete client;
        close(fd);
    }
    s->clients.clear();
    if (s->listen_fd >= 0) close(s->listen_fd);
    if (s->poll_fd   >= 0) close(s->poll_fd);
}                        

Server::~Server() {
    server_stop(this);
}