#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/signal.h>

#include "server.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "%s <port-numner>\n", argv[0]);
        exit(-1);
    }

    srand(time(0));
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    int listenport = atoi(argv[1]);
    Server server(listenport);
    server.interactive();

    return 0;
}
