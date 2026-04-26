#include <cassert>
#include <cstring>
#include <iostream>
#include "../src/resp.h"

static int passed = 0;
static int failed = 0;

#define CHECK(cond, msg) \
    do { \
        if (cond) { std::cout << "  PASS: " << msg << "\n"; passed++; } \
        else      { std::cout << "  FAIL: " << msg << "\n"; failed++; } \
    } while(0)

void test_ping() {
    std::cout << "\n[PING command]\n";
    const char* cmd = "*1\r\n$4\r\nPING\r\n";
    RespResult r = resp_parse(cmd, strlen(cmd));
    CHECK(r.status == RespStatus::OK,     "status is OK");
    CHECK(r.args.size() == 1,             "one argument");
    CHECK(r.args[0] == "PING",            "argument is PING");
    CHECK(r.consumed == strlen(cmd),      "all bytes consumed");
}

void test_set() {
    std::cout << "\n[SET command]\n";
    const char* cmd = "*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n";
    RespResult r = resp_parse(cmd, strlen(cmd));
    CHECK(r.status == RespStatus::OK,     "status is OK");
    CHECK(r.args.size() == 3,             "three arguments");
    CHECK(r.args[0] == "SET",             "args[0] is SET");
    CHECK(r.args[1] == "foo",             "args[1] is foo");
    CHECK(r.args[2] == "bar",             "args[2] is bar");
}

void test_incomplete() {
    std::cout << "\n[incomplete input]\n";
    const char* cmd = "*3\r\n$3\r\nSET\r\n";
    RespResult r = resp_parse(cmd, strlen(cmd));
    CHECK(r.status == RespStatus::INCOMPLETE, "status is INCOMPLETE");
}

void test_one_byte_at_a_time() {
    std::cout << "\n[one byte at a time]\n";
    // This is the critical test from the spec
    const char* cmd = "*1\r\n$4\r\nPING\r\n";
    size_t      len = strlen(cmd);

    RespResult r;
    r.status = RespStatus::INCOMPLETE;
    for (size_t i = 1; i <= len; i++) {
        r = resp_parse(cmd, i);
        if (i < len) {
            CHECK(r.status == RespStatus::INCOMPLETE,
                  std::string("incomplete at byte ") + std::to_string(i));
        }
    }
    CHECK(r.status == RespStatus::OK,  "OK after all bytes");
    CHECK(r.args[0] == "PING",         "correct command after byte-by-byte");
}

void test_two_commands_in_buffer() {
    std::cout << "\n[two commands in one buffer]\n";
    const char* cmd = "*1\r\n$4\r\nPING\r\n*1\r\n$4\r\nPING\r\n";
    size_t      len = strlen(cmd);

    // first parse
    RespResult r1 = resp_parse(cmd, len);
    CHECK(r1.status == RespStatus::OK,   "first command OK");
    CHECK(r1.args[0] == "PING",          "first command is PING");

    // second parse starting after consumed bytes
    RespResult r2 = resp_parse(cmd + r1.consumed, len - r1.consumed);
    CHECK(r2.status == RespStatus::OK,   "second command OK");
    CHECK(r2.args[0] == "PING",          "second command is PING");
}

void test_binary_safe_value() {
    std::cout << "\n[binary safe bulk string]\n";
    const char* cmd = "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$9\r\nhello\r\nhi\r\n";
    RespResult r = resp_parse(cmd, strlen(cmd));
    CHECK(r.status == RespStatus::OK,       "status is OK");
    CHECK(r.args[2].size() == 9,            "value length is 9");
    CHECK(r.args[2] == "hello\r\nhi",       "value preserved with embedded CRLF");
}

void test_inline_ping() {
    std::cout << "\n[inline PING]\n";
    const char* cmd = "PING\r\n";
    RespResult r = resp_parse(cmd, strlen(cmd));
    CHECK(r.status == RespStatus::OK,  "inline PING parsed OK");
    CHECK(r.args[0] == "PING",         "command is PING");
}

int main() {
    std::cout << "=== resp unit tests ===\n";
    test_ping();
    test_set();
    test_incomplete();
    test_one_byte_at_a_time();
    test_two_commands_in_buffer();
    test_binary_safe_value();
    test_inline_ping();

    std::cout << "\n=== results: " << passed << " passed, "
              << failed << " failed ===\n";
    return failed > 0 ? 1 : 0;
}