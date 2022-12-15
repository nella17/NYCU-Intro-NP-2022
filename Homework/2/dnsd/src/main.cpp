#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>

#include "server.hpp"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "%s <port-number> <path/to/the/config/file>\n", argv[0]);
        exit(-1);
    }

    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    auto listenport = (uint16_t)atoi(argv[1]);
    auto config = argv[2];

    Server server(listenport, config);
    server.interactive();

    return 0;
}
