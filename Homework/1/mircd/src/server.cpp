#include "server.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
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
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int connfd = accept(listenfd, (struct sockaddr *) &client_addr, &client_len);
    if (connfd == -1)
        fail("accept");
    int on = 1;
    if ((on = 1) && ioctl(connfd, FIONBIO, &on) < 0)
        fail("ioctl: FIONBIO connfd");

    char* info = sock_info(&client_addr);
    char* host = sock_host(&client_addr);
    controller.client_add(connfd, info, host);

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
        for (char *str = buf, *save; ; str = NULL) {
            char* token = strtok_r(str, "\r\n", &save);
            if (token == NULL)
                break;
            int size = strlen(token);
            if (size == 0)
                continue;
            try {
#ifdef DEBUG
                fprintf(stderr, "%s\n", token);
#endif
                auto cmds = parse(token, size);
                controller.call(connfd, cmds);
            } catch (CMD_MSG cmd) {
                sendcmd(connfd, cmd);
            } catch (EVENT e) {
                switch (e) {
                    case EVENT::DISCONNECT:
                        disconnect(connfd);
                        break;
                }
            }
        }
    }
    return 0;
}

void Server::disconnect(int connfd) {
    if (!controller.client_del(connfd))
        return;

    struct epoll_event ev;
    bzero(&ev, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.fd = listenfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, &ev) == -1)
        fail("epoll_ctl: connfd");

    if (close(connfd) < 0)
        fail("close");
}
