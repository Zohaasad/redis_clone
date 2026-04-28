#include <cassert>
#include <cstring>
#include <iostream>
#include "../src/sds.h"
using namespace std;
static int passed = 0;
static int failed = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { cout << "  PASS: " << msg << "\n"; passed++; } \
        else      { cout << "  FAIL: " << msg << "\n"; failed++; } \
    } while(0)

void test_basic() {
    cout << "\n[basic creation]\n";
    sds s = sds_new_str("hello");
    CHECK(sds_len(s) == 5,           "length is 5");
    CHECK(memcmp(s, "hello", 5) == 0,"content is 'hello'");
    CHECK(s[5] == '\0',              "null terminated");
    sds_free(s);
}

void test_empty() {
    cout << "\n[empty string]\n";
    sds s = sds_empty();
    CHECK(sds_len(s) == 0, "length is 0");
    CHECK(s[0] == '\0',    "null terminated");
    sds_free(s);
}

void test_append() {
     cout << "\n[append]\n";
    sds s = sds_new_str("hello");
    s = sds_append_str(s, " world");
    CHECK(sds_len(s) == 11,                "length after append is 11");
    CHECK(memcmp(s, "hello world", 11) == 0,"content correct after append");
    sds_free(s);
}

void test_binary_safe() {
     cout << "\n[binary safe - embedded null]\n";
    const char data[] = "he\0llo";  
    sds s = sds_new(data, 6);
    CHECK(sds_len(s) == 6,            "length is 6 despite embedded null");
    CHECK(memcmp(s, data, 6) == 0,    "bytes preserved including null");
    sds_free(s);
}

void test_dup() {
    cout << "\n[duplicate]\n";
    sds a = sds_new_str("redis");
    sds b = sds_dup(a);
    CHECK(sds_len(b) == sds_len(a),     "dup has same length");
    CHECK(memcmp(a, b, sds_len(a)) == 0,"dup has same content");
    CHECK(a != b,                        "dup is a different pointer");
    sds_free(a);
    sds_free(b);
}

void test_cmp() {
    cout << "\n[compare]\n";
    sds a = sds_new_str("abc");
    sds b = sds_new_str("abc");
    sds c = sds_new_str("abd");
    CHECK(sds_cmp(a, b) == 0,  "equal strings compare as 0");
    CHECK(sds_cmp(a, c) < 0,   "abc < abd");
    CHECK(sds_cmp(c, a) > 0,   "abd > abc");
    sds_free(a); sds_free(b); sds_free(c);
}

void test_grow() {
    cout << "\n[grow by many appends]\n";
    sds s = sds_empty();
    for (int i = 0; i < 1000; i++) {
        s = sds_append_str(s, "x");
    }
    CHECK(sds_len(s) == 1000, "length is 1000 after 1000 appends");
    sds_free(s);
}

int main() {
    cout << "=== sds unit tests ===\n";
    test_basic();
    test_empty();
    test_append();
    test_binary_safe();
    test_dup();
    test_cmp();
    test_grow();

    cout << "\n=== results: " << passed << " passed, " << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}
