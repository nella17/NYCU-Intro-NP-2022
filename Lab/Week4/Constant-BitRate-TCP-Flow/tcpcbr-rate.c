#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
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
const char HOST[] = "127.0.0.1";
const int PORT = 10003;

const int mss_size = 1500;
const int packet_size = mss_size - 8;
const int header_size = 0x42;
const int buf_size = packet_size - header_size;

void run(double MBps) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        perror("sockfd"), exit(-1);
    /*
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size)))
        perror("setsockopt sendbuf"), exit(-1);
    //*/
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &mss_size, sizeof(mss_size)))
        perror("setsockopt mss"), exit(-1);

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, HOST, &servaddr.sin_addr) <= 0)
        puts("inet_pton"), exit(-1);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        perror("connect"), exit(-1);

    int flag = 0;
    flag |= MSG_DONTWAIT;
    int on;
    if (ioctl(sockfd, FIONBIO, &on) < 0)
        perror("ioctl"), exit(-1);

    char buf[buf_size];
	struct timeval tv0, tv1;
	gettimeofday(&tv0, NULL);
    for (int t = 0; ; ) {
        gettimeofday(&tv1, NULL);
        double d = (tv1.tv_sec-tv0.tv_sec) * 1e6 + (tv1.tv_usec - tv0.tv_usec);
        if (bytesent / d > MBps) {
            usleep(100);
            continue;
        }
        double rate = 1;
        int cursize = buf_size * rate;
        bytesent += cursize + header_size;
        send(sockfd, buf, cursize, flag);
	}
}

signed main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <MBps>", argv[0]);
        exit(-1);
    }

    setup();

    double MBps = atof(argv[1]);
    run(MBps);

    return 0;
}
