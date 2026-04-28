#include "commands.h"
#include "client.h"
#include "sds.h"
#include "object.h"
#include <algorithm>
#include <cctype>
#include <cstring>
using namespace std;
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
static void reply_null_bulk(Client* c) {
    c->write_buf += "$-1\r\n";
}
static void cmd_ping(Client* c, vector<string>& args) {
    if (args.size() == 1) {
        reply_simple(c, "PONG");
    } else {
        reply_bulk(c, args[1].data(), args[1].size());
    }
}
static void cmd_echo(Client* c, vector<string>& args) {
    if (args.size() < 2) {
        reply_error(c, "wrong number of arguments for 'echo'");
        return;
    }
    reply_bulk(c, args[1].data(), args[1].size());
}
static void cmd_set(Client* c,vector<string>& args) {
    if (args.size() < 3) {
        reply_error(c, "wrong number of arguments for 'set'");
        return;
    }
    const string& key = args[1];
    const string& val = args[2];

    sds s   = sds_new(val.data(), val.size());
    Obj* obj = new Obj(s);
    dict_set(g_dict, key.data(), key.size(), obj);
    reply_simple(c, "OK");
}

static void cmd_get(Client* c, vector<string>& args) {
    if (args.size() < 2) {
        reply_error(c, "wrong number of arguments for 'get'");
        return;
    }
    const string& key = args[1];
    Obj* obj = dict_get(g_dict, key.data(), key.size());

    if (!obj) {
        reply_null_bulk(c);
        return;
    }
    if (obj->type != OBJ_STRING) {
        reply_error(c, "WRONGTYPE Operation against a key holding the wrong kind of value");
        return;
    }
    reply_bulk(c, obj->str, sds_len(obj->str));
}

static void cmd_del(Client* c, vector<string>& args) {
    if (args.size() < 2) {
        reply_error(c, "wrong number of arguments for 'del'");
        return;
    }
    long long count = 0;
    for (size_t i = 1; i < args.size(); i++) {
        if (dict_del(g_dict, args[i].data(), args[i].size())) {
            count++;
        }
    }
    reply_integer(c, count);
}
static void cmd_exists(Client* c, vector<string>& args) {
    if (args.size() < 2) {
        reply_error(c, "wrong number of arguments for 'exists'");
        return;
    }
    long long count = 0;
    for (size_t i = 1; i < args.size(); i++) {
        if (dict_get(g_dict, args[i].data(), args[i].size())) {
            count++;
        }
    }
    reply_integer(c, count);
}

static void cmd_dbsize(Client* c,vector<string>& args) {
    (void)args;
    reply_integer(c, (long long)dict_size(g_dict));
}

static void cmd_quit(Client* c, vector<string>& args) {
    (void)args;
    reply_simple(c, "OK");
    c->closing = true;  
}
struct Command {
    const char* name;
    void (*handler)(Client*, std::vector<std::string>&);
};

static Command command_table[] = {
    { "ping",   cmd_ping   },
    { "echo",   cmd_echo   },
    { "set",    cmd_set    },
    { "get",    cmd_get    },
    { "del",    cmd_del    },
    { "exists", cmd_exists },
    { "dbsize", cmd_dbsize },
    { "quit",   cmd_quit   },
    { nullptr,  nullptr    } 
};

void dispatch_command(Client* client, vector<string>& args) {
    if (args.empty()) return;

    string name = args[0];
    for (char& ch : name) ch = (char)tolower((unsigned char)ch);

    for (int i = 0; command_table[i].name; i++) {
        if (name == command_table[i].name) {
            command_table[i].handler(client, args);
            return;
        }
    }
    client->write_buf += "-ERR unknown command '";
    client->write_buf += name;
    client->write_buf += "'\r\n";
}
