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