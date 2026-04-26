#include "commands.h"
#include "client.h"
#include "sds.h"
#include "object.h"
#include <algorithm>
#include <cctype>
#include <cstring>
Dict* g_dict = nullptr;

void commands_init() {
    g_dict = new Dict();
}

static void reply_simple(Client* c, const char* msg) {
    c->write_buf += "+";
    c->write_buf += msg;
    c->write_buf += "\r\n";
}
static void reply_error(Client* c, const char* msg) {
    c->write_buf += "-ERR ";
    c->write_buf += msg;
    c->write_buf += "\r\n";
}
static void reply_integer(Client* c, long long n) {
    c->write_buf += ":";
    c->write_buf += std::to_string(n);
    c->write_buf += "\r\n";
}
static void reply_bulk(Client* c, const char* data, size_t len) {
    c->write_buf += "$";
    c->write_buf += std::to_string(len);
    c->write_buf += "\r\n";
    c->write_buf.append(data, len);
    c->write_buf += "\r\n";
}
