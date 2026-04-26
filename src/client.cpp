#include "client.h"
#include <unistd.h>

Client::Client(int fd) : fd(fd), closing(false) {}

Client::~Client() {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}