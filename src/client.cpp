#include "client.h"
#include <unistd.h>
using namespace std;

Client::Client(int fd) : fd(fd), closing(false) {}

Client::~Client() {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}