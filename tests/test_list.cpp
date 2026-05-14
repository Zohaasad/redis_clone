#include <cassert>
#include <cstring>
#include <iostream>
#include "../src/list.h"

static int passed = 0;
static int failed = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { std::cout << "  PASS: " << msg << "\n"; passed++; } \
        else      { std::cout << "  FAIL: " << msg << "\n"; failed++; } \
    } while(0)

void test_lpush_rpush() {
    std::cout << "\n[lpush and rpush]\n";
    List* l = new List();
    list_rpush(l, sds_new_str("a"));
    list_rpush(l, sds_new_str("b"));
    list_rpush(l, sds_new_str("c"));
    CHECK(list_len(l) == 3, "length is 3 after 3 rpush");
    CHECK(memcmp(list_index(l, 0), "a", 1) == 0, "index 0 is a");
    CHECK(memcmp(list_index(l, 1), "b", 1) == 0, "index 1 is b");
    CHECK(memcmp(list_index(l, 2), "c", 1) == 0, "index 2 is c");
    delete l;
}

void test_lpop_rpop() {
    std::cout << "\n[lpop and rpop]\n";
    List* l = new List();
    list_rpush(l, sds_new_str("x"));
    list_rpush(l, sds_new_str("y"));
    list_rpush(l, sds_new_str("z"));

    sds v = list_lpop(l);
    CHECK(memcmp(v, "x", 1) == 0, "lpop returns x");
    sds_free(v);

    v = list_rpop(l);
    CHECK(memcmp(v, "z", 1) == 0, "rpop returns z");
    sds_free(v);

    CHECK(list_len(l) == 1, "length is 1 after 2 pops");
    delete l;
}

void test_negative_index() {
    std::cout << "\n[negative index]\n";
    List* l = new List();
    list_rpush(l, sds_new_str("a"));
    list_rpush(l, sds_new_str("b"));
    list_rpush(l, sds_new_str("c"));

    CHECK(memcmp(list_index(l, -1), "c", 1) == 0, "index -1 is c");
    CHECK(memcmp(list_index(l, -2), "b", 1) == 0, "index -2 is b");
    CHECK(memcmp(list_index(l, -3), "a", 1) == 0, "index -3 is a");
    CHECK(list_index(l, -4) == nullptr,            "index -4 is null");
    delete l;
}

void test_lrange() {
    std::cout << "\n[lrange]\n";
    List* l = new List();
    list_rpush(l, sds_new_str("a"));
    list_rpush(l, sds_new_str("b"));
    list_rpush(l, sds_new_str("c"));
    list_rpush(l, sds_new_str("d"));

    sds out[10];
    int count = list_range(l, 0, -1, out, 10);
    CHECK(count == 4, "lrange 0 -1 returns 4 elements");
    CHECK(memcmp(out[0], "a", 1) == 0, "out[0] is a");
    CHECK(memcmp(out[3], "d", 1) == 0, "out[3] is d");

    count = list_range(l, 1, 2, out, 10);
    CHECK(count == 2,                  "lrange 1 2 returns 2 elements");
    CHECK(memcmp(out[0], "b", 1) == 0, "out[0] is b");
    CHECK(memcmp(out[1], "c", 1) == 0, "out[1] is c");
    delete l;
}

void test_empty_pop() {
    std::cout << "\n[pop from empty list]\n";
    List* l = new List();
    CHECK(list_lpop(l) == nullptr, "lpop on empty returns null");
    CHECK(list_rpop(l) == nullptr, "rpop on empty returns null");
    delete l;
}

int main() {
    std::cout << "=== list unit tests ===\n";
    test_lpush_rpush();
    test_lpop_rpop();
    test_negative_index();
    test_lrange();
    test_empty_pop();
    std::cout << "\n=== results: " << passed << " passed, " << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}