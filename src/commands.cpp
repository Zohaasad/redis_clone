#include "commands.h"
#include "client.h"
#include "sds.h"
#include "object.h"
#include "list.h"
#include "htable.h"
#include "zset.h"
#include "expiry.h"
#include "rdb.h"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>



Dict* g_dict = nullptr;

void commands_init() {
    g_dict = new Dict();
}

static void reply_simple(Client* c, const char* msg) {
    c->write_buf += "+"; c->write_buf += msg; c->write_buf += "\r\n";
}
static void reply_error(Client* c, const char* msg) {
    c->write_buf += "-ERR "; c->write_buf += msg; c->write_buf += "\r\n";
}
static void reply_integer(Client* c, long long n) {
    c->write_buf += ":"; c->write_buf += std::to_string(n); c->write_buf += "\r\n";
}
static void reply_bulk(Client* c, const char* data, size_t len) {
    c->write_buf += "$"; c->write_buf += std::to_string(len); c->write_buf += "\r\n";
    c->write_buf.append(data, len); c->write_buf += "\r\n";
}
static void reply_null_bulk(Client* c) { c->write_buf += "$-1\r\n"; }
static void reply_array_header(Client* c, size_t n) {
    c->write_buf += "*"; c->write_buf += std::to_string(n); c->write_buf += "\r\n";
}


static void cmd_ping(Client* c, std::vector<std::string>& args) {
    if (args.size() == 1) reply_simple(c, "PONG");
    else reply_bulk(c, args[1].data(), args[1].size());
}

static void cmd_echo(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'echo'"); return; }
    reply_bulk(c, args[1].data(), args[1].size());
}
//person b start
static void cmd_set(Client* c, std::vector<std::string>& args) {
    if (args.size() < 3) { reply_error(c, "wrong number of arguments for 'set'"); return; }
    sds s    = sds_new(args[2].data(), args[2].size());
    Obj* obj = new Obj(s);
    dict_set(g_dict, args[1].data(), args[1].size(), obj);
    reply_simple(c, "OK");
}

static void cmd_get(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'get'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj) { reply_null_bulk(c); return; }
    if (obj->type != OBJ_STRING) { reply_error(c, "WRONGTYPE Operation against a key holding the wrong kind of value"); return; }
    reply_bulk(c, obj->str, sds_len(obj->str));
}

static void cmd_del(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'del'"); return; }
    long long count = 0;
    for (size_t i = 1; i < args.size(); i++)
        if (dict_del(g_dict, args[i].data(), args[i].size())) count++;
    reply_integer(c, count);
}

static void cmd_exists(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'exists'"); return; }
    long long count = 0;
    for (size_t i = 1; i < args.size(); i++) {
        check_expiry(g_dict, args[i].data(), args[i].size());
        if (dict_get(g_dict, args[i].data(), args[i].size())) count++;
    }
    reply_integer(c, count);
}
//person b end
static void cmd_dbsize(Client* c, std::vector<std::string>& args) {
    (void)args;
    reply_integer(c, (long long)dict_size(g_dict));
}

static void cmd_quit(Client* c, std::vector<std::string>& args) {
    (void)args;
    reply_simple(c, "OK");
    c->closing = true;
}


static void cmd_type(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'type'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj) { reply_simple(c, "none"); return; }
    switch (obj->type) {
        case OBJ_STRING: reply_simple(c, "string"); break;
        case OBJ_LIST:   reply_simple(c, "list");   break;
        case OBJ_HASH:   reply_simple(c, "hash");   break;
        case OBJ_ZSET:   reply_simple(c, "zset");   break;
    }
}
static void cmd_expire(Client* c, std::vector<std::string>& args) {
    if (args.size() < 3) { reply_error(c, "wrong number of arguments for 'expire'"); return; }
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj) { reply_integer(c, 0); return; }
    long long secs = std::stoll(args[2]);
    obj->expire_at_ms = now_ms() + secs * 1000;
    reply_integer(c, 1);
}

static void cmd_ttl(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'ttl'"); return; }
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj) { reply_integer(c, -2); return; }
    if (obj->expire_at_ms == 0) { reply_integer(c, -1); return; }
    int64_t remaining_ms = obj->expire_at_ms - now_ms();
    if (remaining_ms <= 0) {
        dict_del(g_dict, args[1].data(), args[1].size());
        reply_integer(c, -2);
        return;
    }
    reply_integer(c, (remaining_ms + 999) / 1000);
}
static void cmd_persist(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'persist'"); return; }
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj || obj->expire_at_ms == 0) { reply_integer(c, 0); return; }
    obj->expire_at_ms = 0;
    reply_integer(c, 1);
}

static void cmd_flushdb(Client* c, std::vector<std::string>& args) {
    (void)args;
    delete g_dict;
    g_dict = new Dict();
    reply_simple(c, "OK");
}

static void cmd_select(Client* c, std::vector<std::string>& args) {
    (void)args;
    reply_simple(c, "OK");   
}


static bool glob_match(const char* pattern, const char* str) {
    while (*pattern) {
        if (*pattern == '*') {
            pattern++;
            if (!*pattern) return true;
            while (*str) {
                if (glob_match(pattern, str)) return true;
                str++;
            }
            return false;
        } else if (*pattern == '?') {
            if (!*str) return false;
            pattern++; str++;
        } else {
            if (*pattern != *str) return false;
            pattern++; str++;
        }
    }
    return *str == '\0';
}

struct KeysCtx {
    const char*              pattern;
    std::vector<std::string> matches;
};

static bool keys_cb(const char* key, size_t klen, Obj* val, void* ud) {
    (void)val;
    KeysCtx* ctx = (KeysCtx*)ud;
    std::string k(key, klen);
    if (glob_match(ctx->pattern, k.c_str())) ctx->matches.push_back(k);
    return true;
}

static void cmd_keys(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'keys'"); return; }
    KeysCtx ctx;
    ctx.pattern = args[1].c_str();
    dict_each(g_dict, keys_cb, &ctx);
    reply_array_header(c, ctx.matches.size());
    for (auto& k : ctx.matches) reply_bulk(c, k.data(), k.size());
}

//person b starts 
static void cmd_setnx(Client* c, std::vector<std::string>& args) {
    if (args.size() < 3) { reply_error(c, "wrong number of arguments for 'setnx'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    if (dict_get(g_dict, args[1].data(), args[1].size())) { reply_integer(c, 0); return; }
    sds s = sds_new(args[2].data(), args[2].size());
    dict_set(g_dict, args[1].data(), args[1].size(), new Obj(s));
    reply_integer(c, 1);
}

static void cmd_append(Client* c, std::vector<std::string>& args) {
    if (args.size() < 3) { reply_error(c, "wrong number of arguments for 'append'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj) {
        sds s = sds_new(args[2].data(), args[2].size());
        dict_set(g_dict, args[1].data(), args[1].size(), new Obj(s));
        reply_integer(c, (long long)args[2].size());
        return;
    }
    if (obj->type != OBJ_STRING) { reply_error(c, "WRONGTYPE Operation against a key holding the wrong kind of value"); return; }
    obj->str = sds_append(obj->str, args[2].data(), args[2].size());
    reply_integer(c, (long long)sds_len(obj->str));
}

static void cmd_strlen(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'strlen'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj) { reply_integer(c, 0); return; }
    if (obj->type != OBJ_STRING) { reply_error(c, "WRONGTYPE Operation against a key holding the wrong kind of value"); return; }
    reply_integer(c, (long long)sds_len(obj->str));
}

static void do_incr(Client* c, const std::string& key, long long delta) {
    check_expiry(g_dict, key.data(), key.size());
    Obj* obj = dict_get(g_dict, key.data(), key.size());
    long long val = 0;
    if (obj) {
        if (obj->type != OBJ_STRING) { reply_error(c, "WRONGTYPE Operation against a key holding the wrong kind of value"); return; }
        char* end;
        val = strtoll(obj->str, &end, 10);
        if (*end != '\0') { reply_error(c, "value is not an integer or out of range"); return; }
    }
    val += delta;
    char buf[32];
    int  len = snprintf(buf, sizeof(buf), "%lld", val);
    sds  s   = sds_new(buf, len);
    if (obj) { sds_free(obj->str); obj->str = s; }
    else     dict_set(g_dict, key.data(), key.size(), new Obj(s));
    reply_integer(c, val);
}

static void cmd_incr(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'incr'"); return; }
    do_incr(c, args[1], 1);
}
static void cmd_incrby(Client* c, std::vector<std::string>& args) {
    if (args.size() < 3) { reply_error(c, "wrong number of arguments for 'incrby'"); return; }
    do_incr(c, args[1], std::stoll(args[2]));
}
static void cmd_decr(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'decr'"); return; }
    do_incr(c, args[1], -1);
}
static void cmd_decrby(Client* c, std::vector<std::string>& args) {
    if (args.size() < 3) { reply_error(c, "wrong number of arguments for 'decrby'"); return; }
    do_incr(c, args[1], -std::stoll(args[2]));
}
//person b ends 

static List* get_or_create_list(const std::string& key) {
    check_expiry(g_dict, key.data(), key.size());
    Obj* obj = dict_get(g_dict, key.data(), key.size());
    if (!obj) {
        List* l  = new List();
        Obj*  no = new Obj(l);
        dict_set(g_dict, key.data(), key.size(), no);
        return l;
    }
    if (obj->type != OBJ_LIST) return nullptr;
    return obj->list;
}

static void cmd_lpush(Client* c, std::vector<std::string>& args) {
    if (args.size() < 3) { reply_error(c, "wrong number of arguments for 'lpush'"); return; }
    List* l = get_or_create_list(args[1]);
    if (!l) { reply_error(c, "WRONGTYPE Operation against a key holding the wrong kind of value"); return; }
    for (size_t i = 2; i < args.size(); i++)
        list_lpush(l, sds_new(args[i].data(), args[i].size()));
    reply_integer(c, (long long)list_len(l));
}

static void cmd_rpush(Client* c, std::vector<std::string>& args) {
    if (args.size() < 3) { reply_error(c, "wrong number of arguments for 'rpush'"); return; }
    List* l = get_or_create_list(args[1]);
    if (!l) { reply_error(c, "WRONGTYPE Operation against a key holding the wrong kind of value"); return; }
    for (size_t i = 2; i < args.size(); i++)
        list_rpush(l, sds_new(args[i].data(), args[i].size()));
    reply_integer(c, (long long)list_len(l));
}

static void cmd_lpop(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'lpop'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj || obj->type != OBJ_LIST) { reply_null_bulk(c); return; }
    sds val = list_lpop(obj->list);
    if (!val) { reply_null_bulk(c); return; }
    reply_bulk(c, val, sds_len(val));
    sds_free(val);
    if (list_len(obj->list) == 0) dict_del(g_dict, args[1].data(), args[1].size());
}

static void cmd_rpop(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'rpop'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj || obj->type != OBJ_LIST) { reply_null_bulk(c); return; }
    sds val = list_rpop(obj->list);
    if (!val) { reply_null_bulk(c); return; }
    reply_bulk(c, val, sds_len(val));
    sds_free(val);
    if (list_len(obj->list) == 0) dict_del(g_dict, args[1].data(), args[1].size());
}

static void cmd_llen(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'llen'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj) { reply_integer(c, 0); return; }
    if (obj->type != OBJ_LIST) { reply_error(c, "WRONGTYPE Operation against a key holding the wrong kind of value"); return; }
    reply_integer(c, (long long)list_len(obj->list));
}

static void cmd_lrange(Client* c, std::vector<std::string>& args) {
    if (args.size() < 4) { reply_error(c, "wrong number of arguments for 'lrange'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj) { reply_array_header(c, 0); return; }
    if (obj->type != OBJ_LIST) { reply_error(c, "WRONGTYPE Operation against a key holding the wrong kind of value"); return; }
    long  start = std::stol(args[2]);
    long  end   = std::stol(args[3]);
    sds   out[65536];
    int   count = list_range(obj->list, start, end, out, 65536);
    reply_array_header(c, count);
    for (int i = 0; i < count; i++) reply_bulk(c, out[i], sds_len(out[i]));
}

static void cmd_lindex(Client* c, std::vector<std::string>& args) {
    if (args.size() < 3) { reply_error(c, "wrong number of arguments for 'lindex'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj || obj->type != OBJ_LIST) { reply_null_bulk(c); return; }
    sds val = list_index(obj->list, std::stol(args[2]));
    if (!val) reply_null_bulk(c);
    else      reply_bulk(c, val, sds_len(val));
}
//person b start
static HTable* get_or_create_hash(const std::string& key) {
    check_expiry(g_dict, key.data(), key.size());
    Obj* obj = dict_get(g_dict, key.data(), key.size());
    if (!obj) {
        HTable* h  = new HTable();
        Obj*    no = new Obj(h);
        dict_set(g_dict, key.data(), key.size(), no);
        return h;
    }
    if (obj->type != OBJ_HASH) return nullptr;
    return obj->hash;
}

static void cmd_hset(Client* c, std::vector<std::string>& args) {
    if (args.size() < 4 || args.size() % 2 != 0) { reply_error(c, "wrong number of arguments for 'hset'"); return; }
    HTable* h = get_or_create_hash(args[1]);
    if (!h) { reply_error(c, "WRONGTYPE Operation against a key holding the wrong kind of value"); return; }
    long long count = 0;
    for (size_t i = 2; i < args.size(); i += 2) {
        bool isnew = !htable_exists(h, args[i].data(), args[i].size());
        htable_set(h, args[i].data(), args[i].size(), args[i+1].data(), args[i+1].size());
        if (isnew) count++;
    }
    reply_integer(c, count);
}

static void cmd_hget(Client* c, std::vector<std::string>& args) {
    if (args.size() < 3) { reply_error(c, "wrong number of arguments for 'hget'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj || obj->type != OBJ_HASH) { reply_null_bulk(c); return; }
    sds val = htable_get(obj->hash, args[2].data(), args[2].size());
    if (!val) reply_null_bulk(c);
    else      reply_bulk(c, val, sds_len(val));
}

static void cmd_hdel(Client* c, std::vector<std::string>& args) {
    if (args.size() < 3) { reply_error(c, "wrong number of arguments for 'hdel'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj || obj->type != OBJ_HASH) { reply_integer(c, 0); return; }
    long long count = 0;
    for (size_t i = 2; i < args.size(); i++)
        if (htable_del(obj->hash, args[i].data(), args[i].size())) count++;
    reply_integer(c, count);
}

static void cmd_hexists(Client* c, std::vector<std::string>& args) {
    if (args.size() < 3) { reply_error(c, "wrong number of arguments for 'hexists'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj || obj->type != OBJ_HASH) { reply_integer(c, 0); return; }
    reply_integer(c, htable_exists(obj->hash, args[2].data(), args[2].size()) ? 1 : 0);
}

static void cmd_hlen(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'hlen'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj || obj->type != OBJ_HASH) { reply_integer(c, 0); return; }
    reply_integer(c, (long long)htable_len(obj->hash));
}

static void cmd_hgetall(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'hgetall'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj || obj->type != OBJ_HASH) { reply_array_header(c, 0); return; }
    sds keys[4096], vals[4096];
    int count = htable_getall(obj->hash, keys, vals, 4096);
    reply_array_header(c, count * 2);
    for (int i = 0; i < count; i++) {
        reply_bulk(c, keys[i], sds_len(keys[i]));
        reply_bulk(c, vals[i], sds_len(vals[i]));
        sds_free(keys[i]);
        sds_free(vals[i]);
    }
}

static void cmd_hkeys(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'hkeys'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj || obj->type != OBJ_HASH) { reply_array_header(c, 0); return; }
    sds keys[4096], vals[4096];
    int count = htable_getall(obj->hash, keys, vals, 4096);
    reply_array_header(c, count);
    for (int i = 0; i < count; i++) {
        reply_bulk(c, keys[i], sds_len(keys[i]));
        sds_free(keys[i]);
        sds_free(vals[i]);
    }
}

static void cmd_hvals(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'hvals'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj || obj->type != OBJ_HASH) { reply_array_header(c, 0); return; }
    sds keys[4096], vals[4096];
    int count = htable_getall(obj->hash, keys, vals, 4096);
    reply_array_header(c, count);
    for (int i = 0; i < count; i++) {
        reply_bulk(c, vals[i], sds_len(vals[i]));
        sds_free(keys[i]);
        sds_free(vals[i]);
    }
}

static ZSet* get_or_create_zset(const std::string& key) {
    check_expiry(g_dict, key.data(), key.size());
    Obj* obj = dict_get(g_dict, key.data(), key.size());
    if (!obj) {
        ZSet* z  = new ZSet();
        Obj*  no = new Obj(z);
        dict_set(g_dict, key.data(), key.size(), no);
        return z;
    }
    if (obj->type != OBJ_ZSET) return nullptr;
    return obj->zset;
}

static void cmd_zadd(Client* c, std::vector<std::string>& args) {
    if (args.size() < 4 || args.size() % 2 != 0) { reply_error(c, "wrong number of arguments for 'zadd'"); return; }
    ZSet* z = get_or_create_zset(args[1]);
    if (!z) { reply_error(c, "WRONGTYPE Operation against a key holding the wrong kind of value"); return; }
    long long added = 0;
    for (size_t i = 2; i < args.size(); i += 2) {
        double score = std::stod(args[i]);
        if (zset_add(z, args[i+1].data(), args[i+1].size(), score)) added++;
    }
    reply_integer(c, added);
}

static void cmd_zscore(Client* c, std::vector<std::string>& args) {
    if (args.size() < 3) { reply_error(c, "wrong number of arguments for 'zscore'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj || obj->type != OBJ_ZSET) { reply_null_bulk(c); return; }
    double score;
    if (!zset_score(obj->zset, args[2].data(), args[2].size(), &score)) { reply_null_bulk(c); return; }
    char buf[32];
    int  len = snprintf(buf, sizeof(buf), "%.17g", score);
    reply_bulk(c, buf, len);
}

static void cmd_zrange(Client* c, std::vector<std::string>& args) {
    if (args.size() < 4) { reply_error(c, "wrong number of arguments for 'zrange'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj || obj->type != OBJ_ZSET) { reply_array_header(c, 0); return; }
    long   start = std::stol(args[2]);
    long   end   = std::stol(args[3]);
    bool   withscores = args.size() >= 5 &&
                        strcasecmp(args[4].c_str(), "WITHSCORES") == 0;
    char*  members[4096];
    double scores[4096];
    int    count = zset_range(obj->zset, start, end, members, scores, 4096);
    reply_array_header(c, withscores ? count * 2 : count);
    for (int i = 0; i < count; i++) {
        reply_bulk(c, members[i], strlen(members[i]));
        if (withscores) {
            char buf[32];
            int  len = snprintf(buf, sizeof(buf), "%.17g", scores[i]);
            reply_bulk(c, buf, len);
        }
    }
}

static void cmd_zrank(Client* c, std::vector<std::string>& args) {
    if (args.size() < 3) { reply_error(c, "wrong number of arguments for 'zrank'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj || obj->type != OBJ_ZSET) { reply_null_bulk(c); return; }
    long rank = zset_rank(obj->zset, args[2].data(), args[2].size());
    if (rank < 0) reply_null_bulk(c);
    else          reply_integer(c, rank);
}

static void cmd_zcard(Client* c, std::vector<std::string>& args) {
    if (args.size() < 2) { reply_error(c, "wrong number of arguments for 'zcard'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj || obj->type != OBJ_ZSET) { reply_integer(c, 0); return; }
    reply_integer(c, (long long)zset_card(obj->zset));
}

static void cmd_zrem(Client* c, std::vector<std::string>& args) {
    if (args.size() < 3) { reply_error(c, "wrong number of arguments for 'zrem'"); return; }
    check_expiry(g_dict, args[1].data(), args[1].size());
    Obj* obj = dict_get(g_dict, args[1].data(), args[1].size());
    if (!obj || obj->type != OBJ_ZSET) { reply_integer(c, 0); return; }
    long long count = 0;
    for (size_t i = 2; i < args.size(); i++)
        if (zset_rem(obj->zset, args[i].data(), args[i].size())) count++;
    reply_integer(c, count);
}


struct Command {
    const char* name;
    void (*handler)(Client*, std::vector<std::string>&);
};


static void cmd_save(Client* c, std::vector<std::string>& args) {
    (void)args;
    bool ok = rdb_save(g_dict, g_rdb_path);
    if (ok)
        reply_simple(c, "OK");
    else
        reply_error(c, "snapshot failed");
}


static void cmd_bgsave(Client* c, std::vector<std::string>& args) {
    (void)args;
    pid_t pid = fork();
    if (pid < 0) {
        reply_error(c, "fork failed");
        return;
    }
    if (pid == 0) {
  
        bool ok = rdb_save(g_dict, g_rdb_path);
        exit(ok ? 0 : 1);
    }
   
    reply_simple(c, "Background saving started");
} // PERSON  b stop 
static Command command_table[] = {

    { "ping",    cmd_ping    },
    { "echo",    cmd_echo    },
    { "set",     cmd_set     },
    { "get",     cmd_get     },
    { "del",     cmd_del     },
    { "exists",  cmd_exists  },
    { "dbsize",  cmd_dbsize  },
    { "quit",    cmd_quit    },
    
    { "type",    cmd_type    },
    { "expire",  cmd_expire  },
    { "ttl",     cmd_ttl     },
    { "persist", cmd_persist },
    { "flushdb", cmd_flushdb },
    { "select",  cmd_select  },
    { "keys",    cmd_keys    },
  
    { "setnx",   cmd_setnx   },
    { "append",  cmd_append  },
    { "strlen",  cmd_strlen  },
    { "incr",    cmd_incr    },
    { "incrby",  cmd_incrby  },
    { "decr",    cmd_decr    },
    { "decrby",  cmd_decrby  },

    { "lpush",   cmd_lpush   },
    { "rpush",   cmd_rpush   },
    { "lpop",    cmd_lpop    },
    { "rpop",    cmd_rpop    },
    { "llen",    cmd_llen    },
    { "lrange",  cmd_lrange  },
    { "lindex",  cmd_lindex  },

    { "hset",    cmd_hset    },
    { "hget",    cmd_hget    },
    { "hdel",    cmd_hdel    },
    { "hexists", cmd_hexists },
    { "hlen",    cmd_hlen    },
    { "hgetall", cmd_hgetall },
    { "hkeys",   cmd_hkeys   },
    { "hvals",   cmd_hvals   },
  
    { "zadd",    cmd_zadd    },
    { "zscore",  cmd_zscore  },
    { "zrange",  cmd_zrange  },
    { "zrank",   cmd_zrank   },
    { "zcard",   cmd_zcard   },
    { "zrem",    cmd_zrem    },
   
    { "save",    cmd_save    },
    { "bgsave",  cmd_bgsave  },
     { nullptr,   nullptr     },
};

void dispatch_command(Client* client, std::vector<std::string>& args) {
    if (args.empty()) return;
    std::string name = args[0];
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






/*
# Connection/Server
PING
ECHO "hello"
DBSIZE
FLUSHDB
SELECT 0
QUIT

# Generic Key
TYPE mykey
EXPIRE mykey 100
TTL mykey
PERSIST mykey
KEYS *

# List
RPUSH mylist "a" "b" "c"
LPUSH mylist "z"
LLEN mylist
LRANGE mylist 0 -1
LINDEX mylist 0
LPOP mylist
RPOP mylist

# Persistence
SAVE
BGSAVE


# String
SET name "Ali"
GET name
DEL name
EXISTS name
SETNX name "Sara"
APPEND name " Khan"
STRLEN name
INCR counter
INCRBY counter 10
DECR counter
DECRBY counter 5

# Hash
HSET user:1 name "Ali" age "25" city "Lahore"
HGET user:1 name
HDEL user:1 city
HEXISTS user:1 name
HLEN user:1
HKEYS user:1
HVALS user:1
HGETALL user:1

# Sorted Set
ZADD leaderboard 100 "Ali"
ZADD leaderboard 85 "Sara"
ZADD leaderboard 92 "Bilal"
ZSCORE leaderboard "Ali"
ZRANK leaderboard "Ali"
ZRANGE leaderboard 0 -1
ZRANGE leaderboard 0 -1 WITHSCORES
ZCARD leaderboard
ZREM leaderboard "Ali"


run from start :

# Start fresh
FLUSHDB

# Person A tests
PING
ECHO "hello"
DBSIZE
SELECT 0
TYPE nonexistent

# List tests
RPUSH mylist "a" "b" "c"
LPUSH mylist "z"
LLEN mylist
LRANGE mylist 0 -1
LINDEX mylist 0
LPOP mylist
RPOP mylist
LRANGE mylist 0 -1

# Expiry tests
SET tempkey "hello"
EXPIRE tempkey 100
TTL tempkey
PERSIST tempkey
TTL tempkey

# Person B string tests
SET name "Ali"
GET name
SETNX name "Sara"
GET name
SETNX newname "Sara"
GET newname
APPEND name " Khan"
GET name
STRLEN name
SET counter 0
INCR counter
INCRBY counter 10
DECR counter
DECRBY counter 5
GET counter

# Person B hash tests
HSET user:1 name "Ali" age "25" city "Lahore"
HGET user:1 name
HGET user:1 age
HEXISTS user:1 name
HEXISTS user:1 fake
HLEN user:1
HKEYS user:1
HVALS user:1
HGETALL user:1
HDEL user:1 city
HGETALL user:1

# Person B sorted set tests
ZADD leaderboard 100 "Ali"
ZADD leaderboard 85 "Sara"
ZADD leaderboard 92 "Bilal"
ZADD leaderboard 78 "Hira"
ZCARD leaderboard
ZSCORE leaderboard "Ali"
ZRANK leaderboard "Ali"
ZRANGE leaderboard 0 -1
ZRANGE leaderboard 0 -1 WITHSCORES
ZREM leaderboard "Hira"
ZRANGE leaderboard 0 -1 WITHSCORES
ZCARD leaderboard

# Keys and type
KEYS *
TYPE name
TYPE mylist
TYPE user:1
TYPE leaderboard

# Persistence
DBSIZE
SAVE



for server :
./minired --port 6380 --data ./dump.rdb

for the other terminal:
redis-cli -p 6380


verify everything loaded :
# Check total keys
DBSIZE

# Check strings
GET name
GET newname
GET counter

# Check list
LRANGE mylist 0 -1

# Check hash
HGETALL user:1

# Check sorted set
ZRANGE leaderboard 0 -1 WITHSCORES

# Check types
TYPE name
TYPE mylist
TYPE user:1
TYPE leaderboard

# Check expiry survived
TTL tempkey




# Set a key with short expiry
SET testkey "hello"
EXPIRE testkey 5
TTL testkey

# Check if sweep deleted it automatically
TTL testkey
GET testkey
*/


