#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <unordered_map>
#include <string>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define MAX_EVENTS 1024
#define MAX_LINE 1024

const int SEND_FLAG = MSG_NOSIGNAL | MSG_DONTWAIT;

void fail(const char* s) {
    if (errno) perror(s);
    else fprintf(stderr, "%s: unknown error", s);
    exit(-1);
}

ssize_t sendstr(int fd, const char* buf) {
    return send(fd, buf, strlen(buf), SEND_FLAG);
}
ssize_t sendstr(int fd, std::string buf) {
    return send(fd, buf.c_str(), buf.size(), SEND_FLAG);
}

char* sock_info(const struct sockaddr_in* sock) {
    char buf[64], host[16];
    sprintf(buf, "%s:%d", inet_ntop(AF_INET, &sock->sin_addr, host, 16), ntohs(sock->sin_port));
    return strdup(buf);
}

struct Client {
    char *info;
    std::string name;
    Client(char* _info = nullptr): info(_info), name("") {}
};

int clicnt = 0;
std::unordered_map<int, Client> cliinfo{};

int handle_new_client(int listenfd) {
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    int connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
    if (connfd == -1)
        fail("accept");
    int on = 1;
    if ((on = 1) && ioctl(connfd, FIONBIO, &on) < 0)
        fail("ioctl: FIONBIO connfd");

    clicnt++;
    char* info = sock_info(&cliaddr);
    printf("* client connected from %s\n", info);
    cliinfo.emplace(connfd, info);

    return connfd;
}

int handle_client_input(int connfd) {
    char buf[MAX_LINE];
    bzero(buf, sizeof(buf));
    int n = read(connfd, buf, MAX_LINE);
    if (n <= 0) {
        close(connfd);
        clicnt--;
        auto cli = cliinfo[connfd];
        printf("* client %s disconnected\n", cli.info);
        cliinfo.erase(connfd);
        free(cli.info);
        return -1;
    } else {
        // TODO
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "%s <port-numner>\n", argv[0]);
        exit(-1);
    }

    srand(time(0));
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    int listenport = atoi(argv[1]);
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) fail("listenfd");

    int on = 1;
    if ((on = 1) && setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof (on)) < 0) {
        fail("setsockopt(SO_REUSEPORT)"); }

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(listenport);

	if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        fail("bind");

	if (listen(listenfd, 1024) < 0)
        fail("listen");

    struct epoll_event ev, events[MAX_EVENTS];
	int epollfd = epoll_create1(0);
	if (epollfd < 0)
		fail("epoll_create1");

    bzero(&ev, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.fd = listenfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) < 0)
		fail("epoll_ctl: listenfd");

	for (;;) {
		int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (nfds == -1)
            fail("epoll_wait");

		for (int i = 0; i < nfds; ++i) {
			if (events[i].data.fd == listenfd) {
                int connfd = handle_new_client(listenfd);
                bzero(&ev, sizeof(ev));
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = connfd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1)
                    fail("epoll_ctl: connfd");
			} else {
				int connfd = events[i].data.fd;
                int stat = handle_client_input(connfd);
                if (stat == -1) {
                    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, &events[i]) == -1)
                        fail("epoll_ctl: connfd");
                }
			}
		}
	}

    return 0;
}
