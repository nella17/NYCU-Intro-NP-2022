#include "server.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "utils.hpp"

Server::Server(int listenport) {
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
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

	epollfd = epoll_create1(0);
	if (epollfd < 0)
		fail("epoll_create1");

    struct epoll_event ev;
    bzero(&ev, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.fd = listenfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) < 0)
		fail("epoll_ctl: listenfd");
}

void Server::interactive() {
    struct epoll_event events[MAX_EVENTS];
	for (;;) {
		int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (nfds == -1)
            fail("epoll_wait");

		for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
			if (fd == listenfd) {
                handle_new_client();
			} else {
                handle_client_input(fd);
			}
		}
	}
}

int Server::handle_new_client() {
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

    struct epoll_event ev;
    bzero(&ev, sizeof(ev));
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = connfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1)
        fail("epoll_ctl: connfd");

    return connfd;
}

int Server::handle_client_input(int connfd) {
    char buf[MAX_LINE];
    bzero(buf, sizeof(buf));
    int n = read(connfd, buf, MAX_LINE);
    if (n <= 0) {
        disconnect(connfd);
        return -1;
    } else {
        // TODO
    }
    return 0;
}

void Server::disconnect(int connfd) {
    auto it = cliinfo.find(connfd);
    if (it == cliinfo.end()) return;

    clicnt--;
    auto cli = it->second;
    printf("* client %s disconnected\n", cli.info);
    cliinfo.erase(it);
    free(cli.info);

    struct epoll_event ev;
    bzero(&ev, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.fd = listenfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, &ev) == -1)
        fail("epoll_ctl: connfd");

    close(connfd);
}
