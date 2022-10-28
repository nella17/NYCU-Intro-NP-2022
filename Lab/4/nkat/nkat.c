#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define set_CLOEXEC(fd) fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC)

void fail(const char* s) {
    if (errno) perror(s);
    else fprintf(stderr, "%s: unknown error", s);
    exit(-1);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "%s <port-numner> /path/to/an/external/program [optional arguments ...]", argv[0]);
        exit(-1);
    }

    signal(SIGCHLD, SIG_IGN);

    int listenport = atoi(argv[1]);
    char* program = argv[2];
    char** optargv = argv+2;

	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) fail("listenfd");

    if (set_CLOEXEC(listenfd) < 0) fail("fcntl");

    int on = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof (on)) < 0)
        fail("setsockopt(SO_REUSEPORT)");

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(listenport);

	if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        fail("bind");

	if (listen(listenfd, 1024) < 0)
        fail("listen");

    while (1) {
        struct sockaddr_in cliaddr;
		socklen_t clilen = sizeof(cliaddr);
		int connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
        if (connfd < 0) {
            perror("accept");
            continue;
        }

		if ((fork()) == 0) {
			close(listenfd);
            int oldstderr = dup(2);
            if (set_CLOEXEC(oldstderr) < 0) fail("fcntl");
            dup2(connfd, 0);
            dup2(connfd, 1);
            dup2(connfd, 2);
            close(connfd);
            execvp(program, optargv);
            dup2(oldstderr, 2);
            fail("exec");
		}

		close(connfd);
        char buf[16];
        printf("New connection from %s:%d\n",
            inet_ntop(AF_INET, &cliaddr.sin_addr, buf, 16), ntohs(cliaddr.sin_port)
        );
	}

    return 0;
}
