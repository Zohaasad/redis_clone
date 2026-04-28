#!/bin/bash
PORT=6380
CLI="redis-cli -p $PORT"
PASS=0
FAIL=0

check() {
    local desc=$1
    local expected=$2
    local actual=$3
    if [ "$actual" = "$expected" ]; then
        echo "  PASS: $desc"
        ((PASS++))
    else
        echo "  FAIL: $desc"
        echo "        expected: $expected"
        echo "        got:      $actual"
        ((FAIL++))
    fi
}

echo "=== Phase 1 tests ==="

check "PING"            "PONG"      "$($CLI PING)"
check "ECHO"            "hello"     "$($CLI ECHO hello)"
check "SET"             "OK"        "$($CLI SET foo bar)"
check "GET existing"    "bar"       "$($CLI GET foo)"
check "GET missing"     ""          "$($CLI GET missing)"
check "EXISTS yes"      "1"         "$($CLI EXISTS foo)"
check "EXISTS no"       "0"         "$($CLI EXISTS missing)"
check "DEL existing"    "1"         "$($CLI DEL foo)"
check "DEL missing"     "0"         "$($CLI DEL foo)"
check "EXISTS deleted"  "0"         "$($CLI EXISTS foo)"
check "SET for DBSIZE"  "OK"        "$($CLI SET a 1)"
check "DBSIZE"          "1"         "$($CLI DBSIZE)"

echo ""
echo "=== results: $PASS passed, $FAIL failed ==="
