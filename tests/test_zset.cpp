#include <cassert>
#include <cstring>
#include <iostream>
#include "../src/zset.h"

static int passed = 0;
static int failed = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { std::cout << "  PASS: " << msg << "\n"; passed++; } \
        else      { std::cout << "  FAIL: " << msg << "\n"; failed++; } \
    } while(0)

void test_add_card() {
    std::cout << "\n[add and cardinality]\n";
    ZSet* z = new ZSet();
    zset_add(z, "Ali",   3, 100.0);
    zset_add(z, "Bilal", 5, 85.0);
    zset_add(z, "Hira",  4, 92.0);
    CHECK(zset_card(z) == 3, "cardinality is 3");
    delete z;
}

void test_score() {
    std::cout << "\n[score lookup]\n";
    ZSet* z = new ZSet();
    zset_add(z, "Ali", 3, 100.0);

    double score;
    CHECK(zset_score(z, "Ali",    3, &score) && score == 100.0, "Ali score is 100");
    CHECK(!zset_score(z, "Nobody", 6, &score),                  "Nobody not found");
    delete z;
}

void test_rank() {
    std::cout << "\n[rank]\n";
    ZSet* z = new ZSet();
    zset_add(z, "Ali",   3, 100.0);
    zset_add(z, "Bilal", 5, 85.0);
    zset_add(z, "Hira",  4, 92.0);

    CHECK(zset_rank(z, "Bilal", 5) == 0, "Bilal rank is 0 lowest score");
    CHECK(zset_rank(z, "Hira",  4) == 1, "Hira rank is 1");
    CHECK(zset_rank(z, "Ali",   3) == 2, "Ali rank is 2 highest score");
    CHECK(zset_rank(z, "Nobody",6) == -1,"Nobody rank is -1");
    delete z;
}

void test_rem() {
    std::cout << "\n[remove]\n";
    ZSet* z = new ZSet();
    zset_add(z, "Ali",   3, 100.0);
    zset_add(z, "Bilal", 5, 85.0);

    CHECK(zset_rem(z, "Ali",    3), "rem Ali returns true");
    CHECK(!zset_rem(z, "Ali",   3), "rem Ali again returns false");
    CHECK(zset_card(z) == 1,        "cardinality is 1 after rem");
    delete z;
}

void test_range() {
    std::cout << "\n[range]\n";
    ZSet* z = new ZSet();
    zset_add(z, "Ali",   3, 100.0);
    zset_add(z, "Bilal", 5, 85.0);
    zset_add(z, "Hira",  4, 92.0);

    char*  members[10];
    double scores[10];
    int count = zset_range(z, 0, 2, members, scores, 10);
    CHECK(count == 3,                        "range 0-2 returns 3");
    CHECK(strcmp(members[0], "Bilal") == 0,  "first is Bilal");
    CHECK(strcmp(members[1], "Hira")  == 0,  "second is Hira");
    CHECK(strcmp(members[2], "Ali")   == 0,  "third is Ali");
    CHECK(scores[0] == 85.0,                 "Bilal score 85");
    CHECK(scores[2] == 100.0,                "Ali score 100");
    delete z;
}

void test_update_score() {
    std::cout << "\n[update score]\n";
    ZSet* z = new ZSet();
    zset_add(z, "Ali", 3, 50.0);
    zset_add(z, "Ali", 3, 99.0);   // update

    CHECK(zset_card(z) == 1, "cardinality still 1 after update");
    double score;
    zset_score(z, "Ali", 3, &score);
    CHECK(score == 99.0, "score updated to 99");
    CHECK(zset_rank(z, "Ali", 3) == 0, "rank is 0 after update");
    delete z;
}

int main() {
    std::cout << "=== zset unit tests ===\n";
    test_add_card();
    test_score();
    test_rank();
    test_rem();
    test_range();
    test_update_score();
    std::cout << "\n=== results: " << passed << " passed, "
              << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}