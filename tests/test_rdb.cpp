#include <cassert>
#include <cstring>
#include <iostream>
#include <cstdio>
#include "../src/rdb.h"
#include "../src/dict.h"
#include "../src/object.h"
#include "../src/sds.h"
#include "../src/list.h"
#include "../src/htable.h"
#include "../src/zset.h"
#include "../src/expiry.h"

static int passed = 0;
static int failed = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { std::cout << "  PASS: " << msg << "\n"; passed++; } \
        else      { std::cout << "  FAIL: " << msg << "\n"; failed++; } \
    } while(0)

#define TEST_RDB_PATH "/tmp/test_minired.rdb"

void test_string_roundtrip() {
    std::cout << "\n[string roundtrip]\n";
    Dict* d = new Dict();

    sds s = sds_new_str("hello world");
    dict_set(d, "greeting", 8, new Obj(s));

    sds s2 = sds_new_str("42");
    dict_set(d, "number", 6, new Obj(s2));

    CHECK(rdb_save(d, TEST_RDB_PATH), "save returns true");
    delete d;

    Dict* d2 = new Dict();
    CHECK(rdb_load(d2, TEST_RDB_PATH), "load returns true");

    Obj* obj = dict_get(d2, "greeting", 8);
    CHECK(obj != nullptr,                          "greeting key exists");
    CHECK(obj && strcmp(obj->str, "hello world") == 0, "greeting value correct");

    Obj* obj2 = dict_get(d2, "number", 6);
    CHECK(obj2 != nullptr,                 "number key exists");
    CHECK(obj2 && strcmp(obj2->str, "42") == 0, "number value correct");

    delete d2;
    remove(TEST_RDB_PATH);
}

void test_list_roundtrip() {
    std::cout << "\n[list roundtrip]\n";
    Dict* d = new Dict();

    List* l = new List();
    list_rpush(l, sds_new_str("a"));
    list_rpush(l, sds_new_str("b"));
    list_rpush(l, sds_new_str("c"));
    dict_set(d, "mylist", 6, new Obj(l));

    CHECK(rdb_save(d, TEST_RDB_PATH), "save returns true");
    delete d;

    Dict* d2 = new Dict();
    CHECK(rdb_load(d2, TEST_RDB_PATH), "load returns true");

    Obj* obj = dict_get(d2, "mylist", 6);
    CHECK(obj != nullptr,                  "mylist exists");
    CHECK(obj && obj->type == OBJ_LIST,    "type is list");
    CHECK(obj && list_len(obj->list) == 3, "list has 3 elements");
    CHECK(obj && strcmp(list_index(obj->list, 0), "a") == 0, "element 0 is a");
    CHECK(obj && strcmp(list_index(obj->list, 1), "b") == 0, "element 1 is b");
    CHECK(obj && strcmp(list_index(obj->list, 2), "c") == 0, "element 2 is c");

    delete d2;
    remove(TEST_RDB_PATH);
}

void test_hash_roundtrip() {
    std::cout << "\n[hash roundtrip]\n";
    Dict* d = new Dict();

    HTable* h = new HTable();
    htable_set(h, "name", 4, "Ayesha", 6);
    htable_set(h, "city", 4, "Lahore", 6);
    dict_set(d, "user:1", 6, new Obj(h));

    CHECK(rdb_save(d, TEST_RDB_PATH), "save returns true");
    delete d;

    Dict* d2 = new Dict();
    CHECK(rdb_load(d2, TEST_RDB_PATH), "load returns true");

    Obj* obj = dict_get(d2, "user:1", 6);
    CHECK(obj != nullptr,               "user:1 exists");
    CHECK(obj && obj->type == OBJ_HASH, "type is hash");
    sds name = htable_get(obj->hash, "name", 4);
    sds city = htable_get(obj->hash, "city", 4);
    CHECK(name && strcmp(name, "Ayesha") == 0, "name is Ayesha");
    CHECK(city && strcmp(city, "Lahore") == 0, "city is Lahore");

    delete d2;
    remove(TEST_RDB_PATH);
}

void test_zset_roundtrip() {
    std::cout << "\n[zset roundtrip]\n";
    Dict* d = new Dict();

    ZSet* z = new ZSet();
    zset_add(z, "Ali",   3, 100.0);
    zset_add(z, "Bilal", 5, 85.0);
    zset_add(z, "Hira",  4, 92.0);
    dict_set(d, "scores", 6, new Obj(z));

    CHECK(rdb_save(d, TEST_RDB_PATH), "save returns true");
    delete d;

    Dict* d2 = new Dict();
    CHECK(rdb_load(d2, TEST_RDB_PATH), "load returns true");

    Obj* obj = dict_get(d2, "scores", 6);
    CHECK(obj != nullptr,               "scores exists");
    CHECK(obj && obj->type == OBJ_ZSET, "type is zset");
    CHECK(obj && zset_card(obj->zset) == 3, "zset has 3 members");

    double score;
    CHECK(zset_score(obj->zset, "Ali",   3, &score) && score == 100.0, "Ali score 100");
    CHECK(zset_score(obj->zset, "Bilal", 5, &score) && score == 85.0,  "Bilal score 85");
    CHECK(zset_score(obj->zset, "Hira",  4, &score) && score == 92.0,  "Hira score 92");

    delete d2;
    remove(TEST_RDB_PATH);
}

void test_expiry_roundtrip() {
    std::cout << "\n[expiry roundtrip]\n";
    Dict* d = new Dict();


    sds s = sds_new_str("temporary");
    Obj* obj = new Obj(s);
    obj->expire_at_ms = now_ms() + 60000;  
    dict_set(d, "temp", 4, obj);

  
    sds s2 = sds_new_str("already gone");
    Obj* obj2 = new Obj(s2);
    obj2->expire_at_ms = now_ms() - 1000; 
    dict_set(d, "dead", 4, obj2);

    CHECK(rdb_save(d, TEST_RDB_PATH), "save returns true");
    delete d;

    Dict* d2 = new Dict();
    CHECK(rdb_load(d2, TEST_RDB_PATH), "load returns true");

    Obj* live = dict_get(d2, "temp", 4);
    CHECK(live != nullptr,                    "non-expired key loaded");
    CHECK(live && live->expire_at_ms != 0,    "expiry preserved");

    Obj* dead = dict_get(d2, "dead", 4);
    CHECK(dead == nullptr, "expired key not loaded");

    delete d2;
    remove(TEST_RDB_PATH);
}

void test_crc_corruption() {
    std::cout << "\n[CRC corruption detection]\n";
    Dict* d = new Dict();
    sds s = sds_new_str("value");
    dict_set(d, "key", 3, new Obj(s));
    rdb_save(d, TEST_RDB_PATH);
    delete d;

  
    FILE* f = fopen(TEST_RDB_PATH, "r+b");
    fseek(f, 10, SEEK_SET);
    fputc(0xff, f);
    fclose(f);

    Dict* d2 = new Dict();
    bool loaded = rdb_load(d2, TEST_RDB_PATH);
    CHECK(!loaded, "corrupt file rejected");
    delete d2;
    remove(TEST_RDB_PATH);
}

int main() {
    std::cout << "=== rdb unit tests ===\n";
    test_string_roundtrip();
    test_list_roundtrip();
    test_hash_roundtrip();
    test_zset_roundtrip();
    test_expiry_roundtrip();
    test_crc_corruption();
    std::cout << "\n=== results: " << passed << " passed, "
              << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}