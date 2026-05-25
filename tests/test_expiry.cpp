#include <cassert>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include "../src/expiry.h"
#include "../src/dict.h"
#include "../src/object.h"
#include "../src/sds.h"

static int passed = 0;
static int failed = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { std::cout << "  PASS: " << msg << "\n"; passed++; } \
        else      { std::cout << "  FAIL: " << msg << "\n"; failed++; } \
    } while(0)

void test_no_expiry() {
    std::cout << "\n[no expiry set]\n";
    Dict* d = new Dict();
    sds s = sds_new_str("hello");
    Obj* obj = new Obj(s);
    obj->expire_at_ms = 0;
    dict_set(d, "key", 3, obj);

    CHECK(!check_expiry(d, "key", 3), "key with no expiry not deleted");
    CHECK(dict_get(d, "key", 3) != nullptr, "key still exists");
    delete d;
}

void test_future_expiry() {
    std::cout << "\n[future expiry]\n";
    Dict* d = new Dict();
    sds s = sds_new_str("hello");
    Obj* obj = new Obj(s);
    obj->expire_at_ms = now_ms() + 60000;   
    dict_set(d, "key", 3, obj);

    CHECK(!check_expiry(d, "key", 3),       "future key not deleted");
    CHECK(dict_get(d, "key", 3) != nullptr, "future key still exists");
    delete d;
}

void test_past_expiry() {
    std::cout << "\n[past expiry]\n";
    Dict* d = new Dict();
    sds s = sds_new_str("hello");
    Obj* obj = new Obj(s);
    obj->expire_at_ms = now_ms() - 1000;   
    dict_set(d, "key", 3, obj);

    CHECK(check_expiry(d, "key", 3),       "expired key detected");
    CHECK(dict_get(d, "key", 3) == nullptr,"expired key deleted");
    delete d;
}

void test_missing_key() {
    std::cout << "\n[missing key]\n";
    Dict* d = new Dict();
    CHECK(!check_expiry(d, "nokey", 5), "missing key returns false");
    delete d;
}

void test_now_ms() {
    std::cout << "\n[now_ms is reasonable]\n";
    int64_t t1 = now_ms();
    usleep(10000);  
    int64_t t2 = now_ms();
    CHECK(t2 > t1,           "time increases");
    CHECK(t2 - t1 >= 10,     "at least 10ms elapsed");
    CHECK(t2 - t1 < 1000,    "less than 1000ms elapsed");
}

void test_actual_expiry() {
    std::cout << "\n[actual expiry after sleep]\n";
    Dict* d = new Dict();
    sds s = sds_new_str("bye");
    Obj* obj = new Obj(s);
    obj->expire_at_ms = now_ms() + 100;   
    dict_set(d, "temp", 4, obj);

    CHECK(!check_expiry(d, "temp", 4),        "not expired yet");
    CHECK(dict_get(d, "temp", 4) != nullptr,  "key exists before expiry");

    usleep(150000); 

    CHECK(check_expiry(d, "temp", 4),         "expired after sleep");
    CHECK(dict_get(d, "temp", 4) == nullptr,  "key gone after expiry");
    delete d;
}

int main() {
    std::cout << "=== expiry unit tests ===\n";
    test_no_expiry();
    test_future_expiry();
    test_past_expiry();
    test_missing_key();
    test_now_ms();
    test_actual_expiry();
    std::cout << "\n=== results: " << passed << " passed, "
              << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}