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

char timebuf[20];

const char SYSTEM[] = "***";
char* msg2log(const char* from, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char msg[MAX_LINE];
    vsprintf(msg, fmt, args);
    va_end(args);

    char frombuf[MAX_LINE/2];
    sprintf(frombuf, from != SYSTEM ? "<%s>" : "%s", from);

    char buf[MAX_LINE*2];
    sprintf(buf, "%s %s %s\n", timebuf, frombuf, msg);
    return strdup(buf);
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
std::unordered_map<int, struct Client> cliinfo{};

void sendexp(int skip, const char* from, const char* fmt, ...) {
    if (fork() == 0) {
        va_list args;
        va_start(args, fmt);
        char msg[MAX_LINE];
        vsprintf(msg, fmt, args);
        va_end(args);
        char* log = msg2log(from, "%s", msg);
        for(auto [fd, cli]: cliinfo)
            if (fd != skip)
                sendstr(fd, log);
        free(log);
        exit(0);
    }
}

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
    char* name = randstr();
    char* info = sock_info(&cliaddr);
    printf("* client connected from %s\n", info);
    sendexp(connfd, SYSTEM, "User <%s> has just landed on the server", name);
    cliinfo[connfd].info = info;
    cliinfo[connfd].name = name;
    char* msg = msg2log(SYSTEM, "Welcome to the simple CHAT server\nTotal %d users online now. Your name is <%s>", clicnt, name);
    sendstr(connfd, msg);
    free(msg);
    return connfd;
}

int handle_client_input(int connfd) {
    char buf[MAX_LINE];
    bzero(buf, sizeof(buf));
    int n = read(connfd, buf, MAX_LINE);
    if (n <= 0) {
        clicnt--;
        char* info = cliinfo[connfd].info;
        char* name = cliinfo[connfd].name;
        cliinfo.erase(connfd);
        printf("* client %s disconnected\n", info);
        sendexp(connfd, SYSTEM, "User <%s> has left the server", name);
        free(info);
        free(name);
        return -1;
    } else {
        if (buf[0] == '/') {
            char* token = strtok(buf, " \n");
            if (!strcmp(token, "/exit")) {
                exit(0);
            } else
            if (!strcmp(token, "/name") && n > 7) {
                char* name = strdup(strtok(buf+6, "\n"));
                char* msg = msg2log(SYSTEM, "Nickname changed to <%s>", name);
                sendstr(connfd, msg);
                free(msg);
                sendexp(connfd, SYSTEM, "User <%s> renamed to <%s>", cliinfo[connfd].name, name);
                free(cliinfo[connfd].name);
                cliinfo[connfd].name = name;
            } else
            if (!strcmp(token, "/who")) {
                if (fork() == 0) {
                    int mxname = 0; // 123.123.123.123:65535 -> 21 char
                    for(auto [fd,cli]: cliinfo) {
                        int len = strlen(cli.name);
                        if (len > mxname) mxname = len;
                    }
                    int len = 2 + mxname + 22 + 1;
                    int size = len * (clicnt+2);
                    char* table = (char*)malloc(size);
                    memset(table, '-', len);
                    table[len-1] = '\n';
                    int k = 0;
                    for(auto [fd,cli]: cliinfo)
                        sprintf(table + len * ++k, "%c %-*s %21s\n",
                            " *"[fd == connfd], mxname, cli.name, cli.info
                        );
                    memset(table + size - len, '-', len);
                    table[size-1] = '\n';
                    send(connfd, table, size, SEND_FLAG);
                    exit(0);
                }
            } else
            {
                char* msg = msg2log(SYSTEM, "Unknown or incomplete command <%s>", token);
                sendstr(connfd, msg);
                free(msg);
            }
        } else {
            char* token = strtok(buf, "\n");
            if (token)
                sendexp(connfd, cliinfo[connfd].name, "%s", token);
        }
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

	for (;;) {
		int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		if (nfds == -1)
            fail("epoll_wait");

        time_t rawtime;
        struct tm * ti;
        time (&rawtime);
        ti = localtime (&rawtime);
        sprintf(timebuf, "%4d-%02d-%02d %02d:%02d:%02d",
            ti->tm_year + 1900, ti->tm_mon+1, ti->tm_mday,
            ti->tm_hour, ti->tm_min, ti->tm_sec
        );

		for (int i = 0; i < nfds; ++i) {
			if (events[i].data.fd == listenfd) {
                int connfd = handle_new_client(listenfd);
                bzero(&ev, sizeof(ev));
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = connfd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1)
                    fail("epoll_ctl: connfd(+)");
			} else {
				int connfd = events[i].data.fd;
                int stat = handle_client_input(connfd);
                if (stat == -1) {
                    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, &events[i]) == -1)
                        fail("epoll_ctl: connfd(-)");
                    close(connfd);
                }
			}
		}
	}

    return 0;
}
