#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <sys/wait.h>
#include "server.h"
#include "commands.h"
#include "rdb.h"

std::string g_rdb_path = "./dump.rdb";

static void print_usage(const char* prog) {
    printf("Usage: %s [--port <port>] [--data <path>]\n", prog);
}


static void sigchld_handler(int) {
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            printf("[minired] background save completed\n");
        else
            fprintf(stderr, "[minired] background save failed\n");
    }
}

int main(int argc, char* argv[]) {
    int port = 6380;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc)
            port = atoi(argv[++i]);
        else if (strcmp(argv[i], "--data") == 0 && i + 1 < argc)
            g_rdb_path = argv[++i];
        else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

  
    signal(SIGCHLD, sigchld_handler);


    commands_init();


    if (!rdb_load(g_dict, g_rdb_path))
        printf("[minired] no snapshot file found, starting empty\n");

 
    Server s;
    if (!server_init(&s, port)) {
        fprintf(stderr, "[minired] failed to start\n");
        return 1;
    }

    server_run(&s);
    return 0;
}
