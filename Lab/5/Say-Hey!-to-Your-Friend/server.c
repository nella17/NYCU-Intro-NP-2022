#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <math.h>
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
#define MAX_FDS 2048
#define MAX_LINE 1024

const int SEND_FLAG = MSG_NOSIGNAL | MSG_DONTWAIT;

void fail(const char* s) {
    if (errno) perror(s);
    else fprintf(stderr, "%s: unknown error", s);
    exit(-1);
}

time_t rawtime;
struct tm * ti;
char timebuf[20];

const char SYSTEM[] = "***";
void sendstr(int fd, const char* from, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char msg[MAX_LINE];
    vsprintf(msg, fmt, args);
    va_end(args);

    char frombuf[MAX_LINE/2];
    sprintf(frombuf, from != SYSTEM ? "<%s>" : "%s", from);

    char buf[MAX_LINE*2];
    int n = sprintf(buf, "%s %s %s\n", timebuf, frombuf, msg);
    send(fd, buf, n, SEND_FLAG);
}

char* sock_info(const struct sockaddr_in* sock) {
    char buf[64], host[16];
    sprintf(buf, "%s:%d", inet_ntop(AF_INET, &sock->sin_addr, host, 16), ntohs(sock->sin_port));
    return strdup(buf);
}
char* randstr() {
    char buf[8];
    for(int i = 0; i < 7; i++)
        buf[i] = rand() % 26 + 'a';
    return strdup(buf);
}

struct Client {
    char *name, *info;
};

int clicnt = 0;
struct Client cliinfo[MAX_FDS] = {};

void sendexp(int skip, const char* from, const char* fmt, ...) {
    if (fork() == 0) {
        va_list args;
        va_start(args, fmt);
        char msg[MAX_LINE];
        vsprintf(msg, fmt, args);
        va_end(args);
        for(int i = 0; i < MAX_FDS; i++)
            if (cliinfo[i].name && i != skip)
                sendstr(i, from, msg);
        exit(0);
    }
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
    if ((on = 1) && setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof (on)) < 0) fail("setsockopt(SO_REUSEPORT)");

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

    char buf[MAX_LINE];
	for (;;) {
		int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (nfds == -1)
            fail("epoll_wait");
        time (&rawtime);
        ti = localtime (&rawtime);
        sprintf(timebuf, "%4d-%02d-%02d %02d:%02d:%02d",
            ti->tm_year + 1900, ti->tm_mon+1, ti->tm_mday,
            ti->tm_hour, ti->tm_min, ti->tm_sec
        );
		for (int i = 0; i < nfds; ++i) {
			if (events[i].data.fd == listenfd) {
                struct sockaddr_in cliaddr;
                socklen_t clilen = sizeof(cliaddr);
				int connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
				if (connfd == -1)
					fail("accept");
                if ((on = 1) && ioctl(connfd, FIONBIO, &on) < 0)
                    fail("ioctl: FIONBIO connfd");
                clicnt++;
                bzero(&ev, sizeof(ev));
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = connfd;
                char* name = randstr();
                char* info = sock_info(&cliaddr);
                printf("* client connected from %s\n", info);
                sendexp(connfd, SYSTEM, "User <%s> has just landed on the server", name);
                cliinfo[connfd].info = info;
                cliinfo[connfd].name = name;
                sendstr(connfd, SYSTEM, "Welcome to the simple CHAT server");
                sendstr(connfd, SYSTEM, "Total %d users online now. Your name is <%s>", clicnt, name);
				if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1)
                    fail("epoll_ctl: connfd");
			} else {
				int connfd = events[i].data.fd;
                bzero(buf, sizeof(buf));
                int n = read(connfd, buf, MAX_LINE);
                if (n <= 0) {
                    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, &events[i]) == -1)
                        fail("epoll_ctl: connfd");
                    close(connfd);
                    clicnt--;
                    char* info = cliinfo[connfd].info;
                    char* name = cliinfo[connfd].name;
                    cliinfo[connfd].info = NULL;
                    cliinfo[connfd].name = NULL;
                    printf("* client %s disconnected\n", info);
                    sendexp(connfd, SYSTEM, "User <%s> has left the server", name);
                    free(info);
                    free(name);
                } else {
                    if (buf[0] == '/') {
                        char* token = strtok(buf, " \n");
                        if (!strcmp(token, "/exit")) {
                            exit(0);
                        } else
                        if (!strcmp(token, "/name") && n > 6) {
                            char* name = strdup(strtok(buf+6, "\n"));
                            sendstr(connfd, SYSTEM, "Nickname changed to <%s>", name);
                            sendexp(connfd, SYSTEM, "User <%s> renamed to <%s>", cliinfo[connfd].name, name);
                            free(cliinfo[connfd].name);
                            cliinfo[connfd].name = name;
                        } else
                        if (!strcmp(token, "/who")) {
                            if (fork() == 0) {
                                int mxname = 0; // 123.123.123.123:65535 -> 21 char
                                for(int i = 0; i < MAX_FDS; i++) if (cliinfo[i].name) {
                                    int len = strlen(cliinfo[i].name);
                                    if (len > mxname) mxname = len;
                                }
                                int len = 2 + mxname + 22 + 1;
                                int size = len * (clicnt+2);
                                char* buf = malloc(size);
                                memset(buf, '-', len);
                                buf[len-1] = '\n';
                                for(int i = 0, j = 1; i < MAX_FDS; i++) if (cliinfo[i].name)
                                    sprintf(buf + len * j++, "%c %-*s %21s\n",
                                        " *"[i == connfd], mxname, cliinfo[i].name, cliinfo[i].info
                                    );
                                memset(buf + size - len, '-', len);
                                buf[size-1] = '\n';
                                send(connfd, buf, size, SEND_FLAG);
                                exit(0);
                            }
                        } else
                        {
                            sendstr(connfd, SYSTEM, "Unknown or incomplete command <%s>", token);
                        }
                    } else {
                        char* token = strtok(buf, "\n");
                        if (token)
                            sendexp(connfd, cliinfo[connfd].name, "%s", token);
                    }
                }
			}
		}
	}

    return 0;
}
