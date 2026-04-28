CXX      = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g

SRC = src/main.cpp \
      src/server.cpp \
      src/client.cpp \
      src/resp.cpp \
      src/sds.cpp \
      src/dict.cpp \
      src/commands.cpp

minired: $(SRC) src/object.h src/sds.h src/dict.h src/client.h src/server.h src/commands.h src/resp.h
	$(CXX) $(CXXFLAGS) -o minired $(SRC)

test_sds: tests/test_sds.cpp src/sds.cpp
	$(CXX) $(CXXFLAGS) -o test_sds tests/test_sds.cpp src/sds.cpp
	./test_sds

test_resp: tests/test_resp.cpp src/resp.cpp src/sds.cpp
	$(CXX) $(CXXFLAGS) -o test_resp tests/test_resp.cpp src/resp.cpp src/sds.cpp
	./test_resp

clean:
	rm -f minired test_sds test_resp
