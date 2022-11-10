#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

constexpr int BUF_SIZE = 65536;
constexpr int MAX_LINE = 1024;

void fail(const char* s) {
    if (errno) perror(s);
    else fprintf(stderr, "%s: unknown error", s);
    exit(-1);
}

constexpr int n = 20;
int control = -1, pid[n];
const char REPORT[] = "/report\n";
const char RESET[] = "/reset\n";

void send(const char* s) {
    send(control, s, strlen(s), 0);
    char buf[MAX_LINE];
    bzero(buf, sizeof(buf));
    int sz = read(control, buf, MAX_LINE);
    write(1, buf, sz);
}

void leave(int /* s */) {
    if (control >= 0) {
        send(REPORT);
        close(control);
        for(int i = 0; i < n; i++)
            kill(pid[i], SIGTERM);
    }
    exit(-1);
}

int connect(char* host, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        perror("sockfd"), exit(-1);

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0)
        puts("inet_pton"), exit(-1);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        perror("connect"), exit(-1);

    return sockfd;
}

signed main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <PORT>", argv[0]);
        exit(-1);
    }

    signal(SIGTERM, leave);

    char* ip = argv[1];
    int port = atoi(argv[2]);

    control = -1;

    for(int i = 0; i < n; i++) {
        if ((pid[i] = fork()) == 0) {
            int connfd = connect(ip, port);
            void* buf = malloc(BUF_SIZE);
            for(;;)
                write(connfd, buf, BUF_SIZE);
        }
    }

    control = connect(ip, port);
    send(RESET);
	for(;;) ;

    return 0;
}
