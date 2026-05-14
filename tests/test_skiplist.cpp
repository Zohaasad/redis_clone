#include <cassert>
#include <cstring>
#include <iostream>
#include "../src/skiplist.h"

static int passed = 0;
static int failed = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { std::cout << "  PASS: " << msg << "\n"; passed++; } \
        else      { std::cout << "  FAIL: " << msg << "\n"; failed++; } \
    } while(0)

void test_insert_and_length() {
    std::cout << "\n[insert and length]\n";
    SkipList* sl = new SkipList();
    sl_insert(sl, "Ali",   3, 100.0);
    sl_insert(sl, "Bilal", 5, 85.0);
    sl_insert(sl, "Hira",  4, 92.0);
    CHECK(sl_length(sl) == 3, "length is 3 after 3 inserts");
    delete sl;
}

void test_get_score() {
    std::cout << "\n[get score]\n";
    SkipList* sl = new SkipList();
    sl_insert(sl, "Ali",   3, 100.0);
    sl_insert(sl, "Bilal", 5, 85.0);

    double score;
    CHECK(sl_get_score(sl, "Ali",     3, &score) && score == 100.0, "Ali score is 100");
    CHECK(sl_get_score(sl, "Bilal",   5, &score) && score == 85.0,  "Bilal score is 85");
    CHECK(!sl_get_score(sl, "Nobody", 6, &score),                   "Nobody not found");
    delete sl;
}

void test_rank() {
    std::cout << "\n[rank]\n";
    SkipList* sl = new SkipList();
    sl_insert(sl, "Ali",   3, 100.0);
    sl_insert(sl, "Bilal", 5, 85.0);
    sl_insert(sl, "Hira",  4, 92.0);

 
    CHECK(sl_get_rank(sl, "Bilal", 5) == 0, "Bilal rank is 0");
    CHECK(sl_get_rank(sl, "Hira",  4) == 1, "Hira rank is 1");
    CHECK(sl_get_rank(sl, "Ali",   3) == 2, "Ali rank is 2");
    delete sl;
}

void test_delete() {
    std::cout << "\n[delete]\n";
    SkipList* sl = new SkipList();
    sl_insert(sl, "Ali",   3, 100.0);
    sl_insert(sl, "Bilal", 5, 85.0);
    sl_insert(sl, "Hira",  4, 92.0);

    CHECK(sl_delete(sl, "Bilal", 5),  "delete Bilal returns true");
    CHECK(sl_length(sl) == 2,         "length is 2 after delete");
    CHECK(sl_get_rank(sl, "Hira", 4) == 0, "Hira rank is now 0");
    CHECK(!sl_delete(sl, "Bilal", 5), "delete again returns false");
    delete sl;
}

void test_range() {
    std::cout << "\n[range by rank]\n";
    SkipList* sl = new SkipList();
    sl_insert(sl, "Ali",   3, 100.0);
    sl_insert(sl, "Bilal", 5, 85.0);
    sl_insert(sl, "Hira",  4, 92.0);

    char*  members[10];
    double scores[10];
    int count = sl_range_by_rank(sl, 0, 2, members, scores, 10);
    CHECK(count == 3,                        "range 0-2 returns 3");
    CHECK(strcmp(members[0], "Bilal") == 0,  "first is Bilal");
    CHECK(strcmp(members[1], "Hira")  == 0,  "second is Hira");
    CHECK(strcmp(members[2], "Ali")   == 0,  "third is Ali");
    CHECK(scores[0] == 85.0,                 "Bilal score 85");
    CHECK(scores[2] == 100.0,                "Ali score 100");
    delete sl;
}

void test_update_score() {
    std::cout << "\n[update score]\n";
    SkipList* sl = new SkipList();
    sl_insert(sl, "Ali", 3, 50.0);
    sl_insert(sl, "Ali", 3, 99.0);  
    CHECK(sl_length(sl) == 1, "length still 1 after update");
    double score;
    sl_get_score(sl, "Ali", 3, &score);
    CHECK(score == 99.0, "score updated to 99");
    delete sl;
}

int main() {
    std::cout << "=== skiplist unit tests ===\n";
    test_insert_and_length();
    test_get_score();
    test_rank();
    test_delete();
    test_range();
    test_update_score();
    std::cout << "\n=== results: " << passed << " passed, " << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}