#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "server.h"
#include "commands.h"
#include"unistd.h"
static void print_usage(const char* prog) {
    printf("Usage: %s [--port <port>]\n", prog);
    printf("  --port  port to listen on (default 6380)\n");
}

int main(int argc, char* argv[]) {
    int port = 6380;

    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    
    commands_init();

    
    Server s;
    if (!server_init(&s, port)) {
        fprintf(stderr, "[minired] failed to start\n");
        return 1;
    }

    server_run(&s);   
}