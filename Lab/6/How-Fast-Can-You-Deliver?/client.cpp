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

constexpr int BUF_SIZE = 65535;
constexpr int MAX_LINE = 1024;

void fail(const char* s) {
    if (errno) perror(s);
    else fprintf(stderr, "%s: unknown error", s);
    exit(-1);
}

constexpr int n = 10;
int epollfd, control, fds[n];
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
    send(REPORT);

    close(epollfd);
    close(control);
    for(int i = 0; i < n; i++)
        close(fds[i]);
    exit(-1);
}

void epoll_add(int fd) {
    struct epoll_event ev;
    bzero(&ev, sizeof(ev));
	ev.events = EPOLLOUT;
	ev.data.fd = fd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) < 0)
		fail("epoll_ctl: fd(+)");
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

    control = connect(ip, port);
    for(int i = 0; i < n; i++)
        fds[i] = connect(ip, port+1);
    send(RESET);

	epollfd = epoll_create1(0);
	if (epollfd < 0)
		fail("epoll_create1");
    for(int i = 0; i < n; i++)
        epoll_add(fds[i]);

    int flag = 0;
    flag |= MSG_DONTWAIT;
    void* buf = malloc(BUF_SIZE);
    struct epoll_event events[n];
	for(;;) {
		int nfds = epoll_wait(epollfd, events, n, -1);
		if (nfds == -1)
            fail("epoll_wait");
        for(int i = 0; i < nfds; i++) {
            int connfd = events[i].data.fd;
            send(connfd, buf, BUF_SIZE, flag);
        }
	}

    return 0;
}
