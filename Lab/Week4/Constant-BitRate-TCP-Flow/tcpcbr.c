#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static struct timeval _t0;
static unsigned long long bytesent = 0;

double tv2s(struct timeval *ptv) {
	return 1.0*(ptv->tv_sec) + 0.000001*(ptv->tv_usec);
}

void handler(int s) {
	struct timeval _t1;
	double t0, t1;
	gettimeofday(&_t1, NULL);
	t0 = tv2s(&_t0);
	t1 = tv2s(&_t1);
	fprintf(stderr, "\n%lu.%06lu %llu bytes sent in %.6fs (%.6f Mbps; %.6f MBps)\n",
		_t1.tv_sec, _t1.tv_usec, bytesent, t1-t0, 8.0*(bytesent/1000000.0)/(t1-t0), (bytesent/1000000.0)/(t1-t0));
	exit(0);
}

void setup() {
	signal(SIGINT,  handler);
	signal(SIGTERM, handler);
	gettimeofday(&_t0, NULL);
}

// const char HOST[] = "inp111.zoolab.org";
const char HOST[] = "140.113.213.213";
const int PORT = 10003;

void run(int size, int time) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        perror("sockfd"), exit(-1);

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, HOST, &servaddr.sin_addr) <= 0)
        puts("inet_pton"), exit(-1);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        perror("connect"), exit(-1);

    int on;
    if (ioctl(sockfd, FIONBIO, &on) < 0)
        perror("ioctl"), exit(-1);

    void* buf = malloc(size);
	struct timeval tv0, tv1;
	gettimeofday(&tv0, NULL);
	for(u_int64_t r = 50; ; r++) {
        gettimeofday(&tv1, NULL);
        uint64_t d = (tv1.tv_sec-tv0.tv_sec) * (uint64_t)1e6 + (tv1.tv_usec - tv0.tv_usec);
        if (d < r * time) {
            struct timespec t = { 0, r * time - d };
            nanosleep(&t, NULL);
        }
        bytesent += size;
        send(sockfd, buf, size, MSG_DONTWAIT);
	}
}

signed main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <MBps>", argv[0]);
        exit(-1);
    }

    setup();

    float MBps = atof(argv[1]);
    int tot = 1460;
    int header = 0x42;
    int size = tot - header;
    int time = tot / MBps;
    run(size, time);

    return 0;
}
