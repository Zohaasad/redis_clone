#include <cassert>
#include <cstring>
#include <iostream>
#include "../src/htable.h"
#include "../src/sds.h"

static int passed = 0;
static int failed = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { std::cout << "  PASS: " << msg << "\n"; passed++; } \
        else      { std::cout << "  FAIL: " << msg << "\n"; failed++; } \
    } while(0)

void test_set_get() {
    std::cout << "\n[set and get]\n";
    HTable* h = new HTable();
    htable_set(h, "name", 4, "Ayesha", 6);
    htable_set(h, "city", 4, "Lahore", 6);

    sds v = htable_get(h, "name", 4);
    CHECK(v && strcmp(v, "Ayesha") == 0, "name is Ayesha");

    sds v2 = htable_get(h, "city", 4);
    CHECK(v2 && strcmp(v2, "Lahore") == 0, "city is Lahore");

    sds v3 = htable_get(h, "phone", 5);
    CHECK(v3 == nullptr, "missing field returns null");

    delete h;
}

void test_overwrite() {
    std::cout << "\n[overwrite field]\n";
    HTable* h = new HTable();
    htable_set(h, "name", 4, "Ayesha", 6);
    htable_set(h, "name", 4, "Bilal",  5);

    sds v = htable_get(h, "name", 4);
    CHECK(v && strcmp(v, "Bilal") == 0, "name overwritten to Bilal");
    CHECK(htable_len(h) == 1,           "length still 1 after overwrite");
    delete h;
}

void test_del() {
    std::cout << "\n[delete field]\n";
    HTable* h = new HTable();
    htable_set(h, "a", 1, "1", 1);
    htable_set(h, "b", 1, "2", 1);

    CHECK(htable_del(h, "a", 1),    "del a returns true");
    CHECK(!htable_del(h, "a", 1),   "del a again returns false");
    CHECK(htable_len(h) == 1,       "length is 1 after del");
    CHECK(!htable_exists(h, "a", 1),"a no longer exists");
    CHECK(htable_exists(h, "b", 1), "b still exists");
    delete h;
}

void test_exists() {
    std::cout << "\n[exists]\n";
    HTable* h = new HTable();
    htable_set(h, "key", 3, "val", 3);
    CHECK(htable_exists(h, "key",     3), "key exists");
    CHECK(!htable_exists(h, "nokey",  5), "nokey does not exist");
    delete h;
}

void test_len() {
    std::cout << "\n[length]\n";
    HTable* h = new HTable();
    CHECK(htable_len(h) == 0, "empty hash length is 0");
    htable_set(h, "a", 1, "1", 1);
    htable_set(h, "b", 1, "2", 1);
    htable_set(h, "c", 1, "3", 1);
    CHECK(htable_len(h) == 3, "length is 3 after 3 sets");
    delete h;
}

void test_getall() {
    std::cout << "\n[getall]\n";
    HTable* h = new HTable();
    htable_set(h, "name", 4, "Ayesha", 6);
    htable_set(h, "age",  3, "27",     2);

    sds keys[10], vals[10];
    int count = htable_getall(h, keys, vals, 10);
    CHECK(count == 2, "getall returns 2 entries");

    for (int i = 0; i < count; i++) {
        sds_free(keys[i]);
        sds_free(vals[i]);
    }
    delete h;
}

int main() {
    std::cout << "=== hash unit tests ===\n";
    test_set_get();
    test_overwrite();
    test_del();
    test_exists();
    test_len();
    test_getall();
    std::cout << "\n=== results: " << passed << " passed, "
              << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}