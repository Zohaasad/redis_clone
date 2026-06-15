# minired — In-Memory Cache Server (Redis Clone)

**Course:** Advanced Database Management — Computer Science, 4th Semester  
**Project:** Project 02 — In-Memory Cache Server (Redis Clone)  
**Group:** 15 
**Members:** Fizzah Amir — Zoha Asad
**Language:** C++17  
**Platform:** macOS (kqueue) and Linux (epoll)

---

## Table of Contents

1. [Overview](#overview)
2. [Build Instructions](#build-instructions)
3. [Running the Server](#running-the-server)
4. [Connecting with redis-cli](#connecting-with-redis-cli)
5. [Implemented Commands](#implemented-commands)
6. [Running Tests](#running-tests)
7. [Benchmark](#benchmark)
8. [Persistence](#persistence)
9. [Architecture](#architecture)
10. [File Structure](#file-structure)
11. [Known Limitations](#known-limitations)

---

## Overview

minired is a small in-memory key-value server that speaks the Redis Serialization
Protocol (RESP). It supports over 40 Redis commands across four core data types:
strings, lists, hashes, and sorted sets. The server is built from scratch in C++17
with no external libraries.

The server runs on a single-threaded event loop using kqueue on macOS and epoll on
Linux. An unmodified redis-cli connects to it and works exactly as with real Redis.
Data is persisted to disk via fork-based binary snapshots with CRC64 verification.

Key design goals taken from Redis itself:
- Single-threaded execution — no locks, no contention
- Non-blocking I/O — one thread handles thousands of clients
- Binary-safe strings — sds type stores length alongside bytes
- Hand-implemented data structures — skip list, hash table, linked list
- Asynchronous persistence — fork() isolates snapshot from live traffic

---

## Build Instructions

### Requirements

| Tool | macOS | Linux |
|------|-------|-------|
| C++17 compiler | Apple Clang 15+ | g++ 11+ |
| make | GNU Make 3.81+ | GNU Make 4+ |
| redis-cli | brew install redis | sudo apt install redis-tools |
| redis-benchmark | included with redis | included with redis-tools |

### macOS Setup

```bash
# install Homebrew if not installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# install redis tools
brew install redis

# build
make
```

### Linux Setup

```bash
# install build tools and redis client
sudo apt update
sudo apt install -y build-essential redis-tools

# build
make
```

### Build Commands

```bash
# build the server
make

# clean build artifacts
make clean

# rebuild from scratch
make clean && make
```

---

## Running the Server

```bash
# start on default port 6380
./minired

# start on custom port
./minired --port 6380

# start with persistence file
./minired --port 6380 --data ./dump.rdb
```

Server output on startup:

[minired] starting on port 6380
[minired] snapshot loaded from ./dump.rdb (42 keys)
[minired] event loop ready

Or if no snapshot exists:
[minired] starting on port 6380
[minired] no snapshot file found, starting empty
[minired] event loop ready

Stop the server with Ctrl+C.

---

## Connecting with redis-cli

```bash
# interactive session
redis-cli -p 6380

# single command
redis-cli -p 6380 PING

# run a session
redis-cli -p 6380
127.0.0.1:6380> PING
PONG
127.0.0.1:6380> SET name "Ayesha"
OK
127.0.0.1:6380> GET name
"Ayesha"
127.0.0.1:6380> SET counter 0
OK
127.0.0.1:6380> INCR counter
(integer) 1
127.0.0.1:6380> INCR counter
(integer) 2
127.0.0.1:6380> INCRBY counter 10
(integer) 12
127.0.0.1:6380> LPUSH tasks "wash dishes" "buy bread" "call mom"
(integer) 3
127.0.0.1:6380> LRANGE tasks 0 -1
1) "call mom"
2) "buy bread"
3) "wash dishes"
127.0.0.1:6380> HSET user:1 name "Ayesha" age 27 city "Lahore"
(integer) 3
127.0.0.1:6380> HGETALL user:1
1) "name"
2) "Ayesha"
3) "age"
4) "27"
5) "city"
6) "Lahore"
127.0.0.1:6380> ZADD leaderboard 100 "Ali" 85 "Bilal" 92 "Hira"
(integer) 3
127.0.0.1:6380> ZRANGE leaderboard 0 -1 WITHSCORES
1) "Bilal"
2) "85"
3) "Hira"
4) "92"
5) "Ali"
6) "100"
127.0.0.1:6380> EXPIRE name 60
(integer) 1
127.0.0.1:6380> TTL name
(integer) 58
127.0.0.1:6380> SAVE
OK
127.0.0.1:6380> DBSIZE
(integer) 5
127.0.0.1:6380> QUIT
OK
```

---

## Implemented Commands

### Connection Commands

| Command | Description | Returns |
|---------|-------------|---------|
| `PING [message]` | ping the server | PONG or message |
| `ECHO message` | echo a message back | message |
| `QUIT` | close the connection | OK |
| `SELECT index` | select database (ignored, single db) | OK |

### Generic Key Commands

| Command | Description | Returns |
|---------|-------------|---------|
| `DEL key [key ...]` | delete one or more keys | number deleted |
| `EXISTS key [key ...]` | check if keys exist | count of existing keys |
| `TYPE key` | return the type of a key | string/list/hash/zset/none |
| `EXPIRE key seconds` | set a TTL in seconds | 1 if set, 0 if key missing |
| `TTL key` | get remaining TTL in seconds | seconds, -1 no expiry, -2 missing |
| `PERSIST key` | remove TTL from a key | 1 if removed, 0 if no TTL |
| `KEYS pattern` | find all keys matching pattern | array of keys |
| `DBSIZE` | number of keys in the database | integer |
| `FLUSHDB` | delete all keys | OK |

### String Commands

| Command | Description | Returns |
|---------|-------------|---------|
| `SET key value` | set a string value | OK |
| `GET key` | get a string value | value or nil |
| `SETNX key value` | set only if key does not exist | 1 if set, 0 if exists |
| `APPEND key value` | append to a string | new length |
| `STRLEN key` | get the length of a string | integer |
| `INCR key` | increment integer value by 1 | new value |
| `INCRBY key n` | increment integer value by n | new value |
| `DECR key` | decrement integer value by 1 | new value |
| `DECRBY key n` | decrement integer value by n | new value |

### List Commands

| Command | Description | Returns |
|---------|-------------|---------|
| `LPUSH key value [value ...]` | push values to the head | new length |
| `RPUSH key value [value ...]` | push values to the tail | new length |
| `LPOP key` | pop value from the head | value or nil |
| `RPOP key` | pop value from the tail | value or nil |
| `LLEN key` | get the length of a list | integer |
| `LRANGE key start stop` | get a range of elements | array |
| `LINDEX key index` | get element by index | value or nil |

### Hash Commands

| Command | Description | Returns |
|---------|-------------|---------|
| `HSET key field value [field value ...]` | set one or more fields | number of new fields |
| `HGET key field` | get a field value | value or nil |
| `HDEL key field [field ...]` | delete one or more fields | number deleted |
| `HEXISTS key field` | check if a field exists | 1 or 0 |
| `HLEN key` | get the number of fields | integer |
| `HKEYS key` | get all field names | array |
| `HVALS key` | get all values | array |
| `HGETALL key` | get all fields and values | array of field value pairs |

### Sorted Set Commands

| Command | Description | Returns |
|---------|-------------|---------|
| `ZADD key score member [score member ...]` | add members with scores | number added |
| `ZSCORE key member` | get the score of a member | score or nil |
| `ZRANGE key start stop [WITHSCORES]` | get range by rank | array |
| `ZRANK key member` | get the rank of a member | integer or nil |
| `ZCARD key` | get the number of members | integer |
| `ZREM key member [member ...]` | remove one or more members | number removed |

### Persistence Commands

| Command | Description | Returns |
|---------|-------------|---------|
| `SAVE` | synchronous snapshot to disk | OK |
| `BGSAVE` | asynchronous background snapshot | Background saving started |

---

## Running Tests

Run each test suite individually:

```bash
# sds binary-safe string type
make test_sds
# expected: 16/16 passed

# RESP protocol parser
make test_resp
# expected: 34/34 passed

# doubly linked list
make test_list
# expected: 19/19 passed

# skip list data structure
make test_skiplist
# expected: 19/19 passed

# hash table data type
make test_hash

# sorted set data type
make test_zset

# TTL and expiry logic
make test_expiry

# snapshot save and load roundtrip
make test_rdb
```

Run all tests at once:

```bash
make test_sds && make test_resp && make test_list && \
make test_skiplist && make test_hash && make test_zset && \
make test_expiry && make test_rdb
```

---

## Benchmark

Start the server first:

```bash
./minired --port 6380
```

Run the full benchmark suite:

```bash
chmod +x benchmark/run_benchmark.sh
./benchmark/run_benchmark.sh
```

Or run individual benchmarks:

```bash
# baseline ping throughput
redis-benchmark -p 6380 -t ping -n 100000

# string operations
redis-benchmark -p 6380 -t set,get -n 100000 -r 10000

# list operations
redis-benchmark -p 6380 -t lpush,lpop -n 50000

# hash operations
redis-benchmark -p 6380 -t hset -n 50000

# sorted set operations
redis-benchmark -p 6380 -t zadd -n 50000
```

### Our Results (MacBook Pro, Apple M2)

| Operation | Throughput | p50 | p95 | p99 |
|-----------|-----------|-----|-----|-----|
| PING | 205,338 ops/sec | 0.127ms | 0.191ms | 0.327ms |
| SET | 200,000 ops/sec | 0.135ms | 0.159ms | 0.471ms |
| GET | 209,643 ops/sec | 0.127ms | 0.143ms | 0.151ms |

Minimum requirement: 50,000 ops/sec. We achieved 4x the requirement.

---

## Persistence

minired saves data using fork-based binary snapshots.

### How it works

1. The server calls `fork()`
2. The child process inherits a copy-on-write snapshot of memory
3. The child serializes every key to a binary file and exits
4. The parent continues serving clients with zero downtime
5. On restart the server reads the snapshot and rebuilds the dictionary

### Snapshot format
Offset  Size  Field

0       4     magic bytes 'MRDB'
4       4     version (uint32 = 1)
8       8     key count (uint64)
--- for each key ---
1     type (0=string 1=list 2=hash 3=zset)
8     expire_at_ms (int64, 0 = no expiry)
4     key length
N     key bytes
...   type-specific body
--- end ---
8     CRC64 checksum of everything above

The CRC64 checksum protects against corruption. If the checksum does not match on load,
the server refuses to start and prints an error.

### Manual snapshot

```bash
# synchronous — waits for completion
redis-cli -p 6380 SAVE

# asynchronous — returns immediately
redis-cli -p 6380 BGSAVE
```

### Persistence correctness test

```bash
# start fresh server
./minired --port 6380 --data ./dump.rdb

# insert data
redis-cli -p 6380 SET greeting "hello"
redis-cli -p 6380 LPUSH tasks "a" "b" "c"
redis-cli -p 6380 HSET user:1 name "Ayesha" age 27
redis-cli -p 6380 ZADD scores 100 "Ali" 85 "Bilal"

# save
redis-cli -p 6380 SAVE

# stop server
pkill minired

# restart
./minired --port 6380 --data ./dump.rdb

# verify all data is still there
redis-cli -p 6380 GET greeting
redis-cli -p 6380 LRANGE tasks 0 -1
redis-cli -p 6380 HGETALL user:1
redis-cli -p 6380 ZRANGE scores 0 -1 WITHSCORES
```

---

## Architecture

### Event Loop

The server uses a single-threaded event loop. All network I/O, command execution,
and data structure manipulation happen on one thread with no locks. This works because:

- Sockets are set to non-blocking mode with `O_NONBLOCK`
- `kqueue` (macOS) or `epoll` (Linux) notifies when a socket is ready
- `EPOLLOUT` / `EVFILT_WRITE` is only registered when there is pending output
- No handler function ever blocks

### RESP Parser

The RESP parser is incremental. Each client has a read buffer. When `recv` returns
data it is appended to the buffer. The parser consumes as many complete commands as
possible and returns `INCOMPLETE` if more bytes are needed. This handles TCP
fragmentation correctly — a command split across multiple `recv` calls is reassembled
transparently.

### Data Structures

| Type | Implementation | Complexity |
|------|---------------|------------|
| String | sds (simple dynamic string) | O(1) get/set |
| Dictionary | chaining hash table, FNV-1a hash | O(1) avg get/set/del |
| List | doubly linked list | O(1) push/pop, O(n) index |
| Hash | dict wrapping sds→sds | O(1) field get/set |
| Sorted Set | skip list + hash table | O(log n) add/remove/rank |

### sds Strings

sds is a binary-safe string type. The header (length + capacity) is stored immediately
before the char* pointer so the pointer can be passed to any function expecting
const char* while length queries read backward from the pointer via `sds_get_header(s)`.

### Skip List

The sorted set is backed by a skip list — a probabilistic alternative to a balanced
tree. Each node has a random number of forward pointers. Search starts at the highest
level and descends, giving O(log n) expected time for insert, delete, and search.
A hash table maps member → score for O(1) score lookup by member name.

### Lazy Expiration

Every command that reads or writes a key first calls `check_expiry()`. If the key's
`expire_at_ms` field is non-zero and less than the current time, the key is deleted
on the spot and the command proceeds as if the key never existed. No background thread
is needed.

### Fork-Based Snapshots

`SAVE` and `BGSAVE` both call `fork()`. The child inherits a copy-on-write snapshot
of the parent's memory. Any writes the parent makes after the fork modify pages that
are copied on write — the child's view remains frozen at the moment of the fork. The
child writes the snapshot file, renames it atomically into place, and exits. The
parent either waits (`SAVE`) or continues immediately (`BGSAVE`).

---

## File Structure
redis_clone/
├── Makefile
├── README.md
├── dump.rdb               (created by SAVE)
├── benchmark/
│   └── run_benchmark.sh
├── src/
│   ├── main.cpp           entry point, arg parsing, startup
│   ├── server.h / .cpp    event loop (epoll/kqueue)
│   ├── client.h / .cpp    per-client state and buffers
│   ├── resp.h / .cpp      RESP protocol parser and writer
│   ├── sds.h / .cpp       binary-safe dynamic string
│   ├── dict.h / .cpp      hash table (string → Obj)
│   ├── object.h           Obj struct and ObjType enum
│   ├── commands.h / .cpp  command dispatch and all handlers
│   ├── list.h / .cpp      doubly linked list
│   ├── expiry.h / .cpp    TTL and lazy expiration
│   ├── htable.h / .cpp    hash data type
│   ├── skiplist.h / .cpp  skip list
│   ├── zset.h / .cpp      sorted set (skip list + hash table)
│   └── rdb.h / .cpp       binary snapshot save and load
└── tests/
├── phase1_test.sh
├── test_sds.cpp        16 tests
├── test_resp.cpp       34 tests
├── test_list.cpp       19 tests
├── test_skiplist.cpp   19 tests
├── test_hash.cpp
├── test_zset.cpp
├── test_expiry.cpp
└── test_rdb.cpp

---

## Known Limitations

- Single database only — `SELECT` is accepted but ignored
- No replication or clustering
- No Lua scripting
- No pub/sub messaging
- `KEYS` pattern supports `*` and `?` only — no character classes like `[abc]`
- No `MSET`, `MGET`, `GETSET` commands
- No `LSET`, `LTRIM`, `LPOS` commands
- No `HSETNX`, `HINCRBY` commands
- No `ZRANGEBYSCORE`, `ZRANGEBYLEX` commands
- Snapshot is written in native byte order — not portable across architectures
- No automatic periodic snapshots — must call `SAVE` or `BGSAVE` manually
