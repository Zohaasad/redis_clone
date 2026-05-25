CXX      = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g

SRC = src/main.cpp \
      src/server.cpp \
      src/client.cpp \
      src/resp.cpp \
      src/sds.cpp \
      src/dict.cpp \
      src/commands.cpp \
      src/list.cpp \
      src/expiry.cpp \
      src/htable.cpp \
      src/skiplist.cpp \
      src/zset.cpp \
      src/rdb.cpp

minired: $(SRC)
	$(CXX) $(CXXFLAGS) -o minired $(SRC)

test_sds: tests/test_sds.cpp src/sds.cpp
	$(CXX) $(CXXFLAGS) -o test_sds tests/test_sds.cpp src/sds.cpp
	./test_sds

test_resp: tests/test_resp.cpp src/resp.cpp src/sds.cpp
	$(CXX) $(CXXFLAGS) -o test_resp tests/test_resp.cpp src/resp.cpp src/sds.cpp
	./test_resp

test_list: tests/test_list.cpp src/list.cpp src/sds.cpp
	$(CXX) $(CXXFLAGS) -o test_list tests/test_list.cpp src/list.cpp src/sds.cpp
	./test_list

test_skiplist: tests/test_skiplist.cpp src/skiplist.cpp
	$(CXX) $(CXXFLAGS) -o test_skiplist tests/test_skiplist.cpp src/skiplist.cpp
	./test_skiplist

test_rdb: tests/test_rdb.cpp src/rdb.cpp src/sds.cpp src/dict.cpp \
          src/list.cpp src/htable.cpp src/skiplist.cpp src/zset.cpp src/expiry.cpp
	$(CXX) $(CXXFLAGS) -o test_rdb tests/test_rdb.cpp src/rdb.cpp src/sds.cpp \
	       src/dict.cpp src/list.cpp src/htable.cpp src/skiplist.cpp \
	       src/zset.cpp src/expiry.cpp
	./test_rdb

clean:
	rm -f minired test_sds test_resp test_list test_skiplist test_rdb

.PHONY: clean test_sds test_resp test_list test_skiplist test_rdb