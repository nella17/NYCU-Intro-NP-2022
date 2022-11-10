#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <math.h>
#include <unordered_map>
#include <chrono>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define bzero(buf, len) memset(buf, 0, len)
using ll = long long;
using ld = long double;

void fail(const char* s) {
    if (errno) perror(s);
    else fprintf(stderr, "%s: unknown error", s);
    exit(-1);
}

int create(int listenport) {
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) fail("listenfd");

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	// servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0) {
        fail("inet_pton"); }
	servaddr.sin_port        = htons(listenport);

	if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        fail("bind");

	if (listen(listenfd, 1024) < 0)
        fail("listen");

    return listenfd;
}

void epoll_add(int epollfd, int fd) {
    struct epoll_event ev;
    bzero(&ev, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) < 0)
		fail("epoll_ctl: fd(+)");
}

void epoll_del(int epollfd, struct epoll_event ev) {
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, ev.data.fd, &ev) == -1)
        fail("epoll_ctl: fd(-)");
}

ld gettime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ld time = tv.tv_sec + ld(0.000001) * (tv.tv_usec);
    return time;
}

constexpr int MAX_EVENTS = 1024;
constexpr int MAX_LINE = 1024;
constexpr int MAX_DATA_LINE = 65536;
constexpr int SEND_FLAG = MSG_NOSIGNAL | MSG_DONTWAIT;

int serverfd, sinkfd, epollfd;
std::unordered_map<int, int> mapping{}, cnt{}, mss{};

const int header_size = 0x42;

int handle_new_client(int listenfd) {
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    int connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
    if (connfd == -1)
        fail("accept");
    int mss_size;
    socklen_t size = sizeof(mss_size);
    if (getsockopt(connfd, IPPROTO_TCP, TCP_MAXSEG, &mss_size, &size)) {
        fail("getsockopt(TCP_MAXSEG)"); }
    int on = 1;
    if ((on = 1) && ioctl(connfd, FIONBIO, &on) < 0)
        fail("ioctl: FIONBIO connfd");
    cnt[listenfd]++;
    mapping.emplace(connfd, listenfd);
    mss.emplace(connfd, mss_size);
    return connfd;
}

void handle_del_client(int connfd, struct epoll_event ev) {
    epoll_del(epollfd, ev);
    auto it = mapping.find(connfd);
    cnt[it->second]--;
    mapping.erase(it);
    close(connfd);
}

ll recv_size = 0;
ld start_time = gettime(), current_time;

ssize_t sendstr(int fd, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char msg[MAX_LINE];
    vsprintf(msg, fmt, args);
    va_end(args);

    char buf[MAX_LINE*2];
    sprintf(buf, "%.6Lf %s", current_time, msg);
    return send(fd, buf, strlen(buf), SEND_FLAG);
}

int handle_server(int connfd) {
    char buf[MAX_LINE];
    bzero(buf, sizeof(buf));
    int n = read(connfd, buf, MAX_LINE);
    if (n <= 0) return -1;
    for (char *str = buf, *save; ; str = NULL) {
        char* token = strtok_r(str, "\n", &save);
        if (token == NULL) break;
        if (!strcmp(token, "/reset")) {
            sendstr(connfd, "RESET %lld\n", recv_size);
            recv_size = 0;
            start_time = current_time;
        } else
        if (!strcmp(token, "/ping")) {
            sendstr(connfd, "PONG\n");
        } else
        if (!strcmp(token, "/report")) {
            ld dt = current_time - start_time;
            ld rate = (8 * recv_size) / dt / (1<<20);
            sendstr(connfd, "REPORT %lld %.6LFs %.6LfMbps\n", recv_size, dt, rate);
        } else
        if (!strcmp(token, "/clients")) {
            sendstr(connfd, "CLIENTS %d\n", cnt[sinkfd]);
        } else
        if (!strcmp(token, "/exit")) {
            exit(0);
        } else
            ;
    }
    return 0;
}

int handle_sink(int connfd) {
    int buf_size = mss[connfd] - header_size;
    char buf[MAX_DATA_LINE];
    int n = read(connfd, buf, buf_size);
    if (n <= 0) return -1;
    recv_size += header_size + n;
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "%s <port-numner>\n", argv[0]);
        exit(-1);
    }

    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    u_int16_t listenport = atol(argv[1]);
    serverfd = create(listenport);
    sinkfd = create(listenport+1);

	epollfd = epoll_create1(0);
	if (epollfd < 0)
		fail("epoll_create1");

    epoll_add(epollfd, serverfd);
    epoll_add(epollfd, sinkfd);

    struct epoll_event events[MAX_EVENTS];
	for (;;) {
		int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (nfds == -1)
            fail("epoll_wait");

        current_time = gettime();

		for (int i = 0; i < nfds; ++i) {
            int connfd = events[i].data.fd;
			if (connfd == serverfd) {
                connfd = handle_new_client(serverfd);
                epoll_add(epollfd, connfd);
			} else if (connfd == sinkfd) {
                connfd = handle_new_client(sinkfd);
                epoll_add(epollfd, connfd);
			} else {
				int listenfd = mapping.find(connfd)->second;
                int stat;
                if (listenfd == serverfd)
                    stat = handle_server(connfd);
                else
                    stat = handle_sink(connfd);
                if (stat < 0)
                    handle_del_client(connfd, events[i]);
			}
		}
	}

    return 0;
}
