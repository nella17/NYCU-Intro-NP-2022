#include <cstdint>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
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
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <vector>
#include <set>

#include "header.hpp"
#include "util.hpp"

int connect(char* host, uint16_t port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0)
        perror("sockfd"), exit(-1);

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0)
        fail("inet_pton");

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        fail("connect");

    return sockfd;
}

inline void wrap_send(uint32_t seq, int sockfd, const void* buf, size_t len) {
    sender_hdr_t hdr;
    bzero(&hdr, PACKET_SIZE);
    hdr.data_seq = seq;
    memcpy(hdr.data, buf, len);
    hdr.checksum = adler32(hdr.data, DATA_SIZE);
    // dump_sender_hdr(&hdr);
    for(int i = 0; i < 10; i++)
        send(sockfd, &hdr, PACKET_SIZE, 0);
}

int main(int argc, char *argv[]) {
    if (argc != 5)
        return -fprintf(stderr, "Usage: %s <path-to-read-files> <total-number-of-files> <port> <server-ip-address>", argv[0]);

    setvbufs();

    char* path = argv[1];
    uint32_t total = (uint32_t)atoi(argv[2]);
    uint16_t port = (uint16_t)atoi(argv[3]);
    char* ip = argv[4];

    std::vector<file_t> files(total);

    for(uint32_t idx = 0; idx < total; idx++) {
        char filename[256];
        sprintf(filename, "%s/%06d", path, idx);
        int filefd = open(filename, O_RDONLY);
        if (filefd < 0) fail("open");
        auto &file = files[idx];
        file.filename = idx;
        file.size = (uint32_t)lseek(filefd, 0, SEEK_END);
        file.data = (char*)malloc(file.size);
        lseek(filefd, 0, SEEK_SET);
        read(filefd, file.data, file.size);
        close(filefd);
        fprintf(stderr, "read '%s' (%d bytes)\n", filename, file.size);
    }

    int connfd = connect(ip, port);
    for(auto &file: files) {
        init_t init {
            .filename = file.filename,
            .filesize = file.size,
        };
        while (true) {
            wrap_send(0, connfd, &init, sizeof(init));
            response_hdr_t res;
            if (recv(connfd, &res, sizeof(res), 0) != sizeof(res))
                continue;
            if (res.flag_check ^ RES_MAGIC ^ res.flag)
                continue;
            if (res.flag & RES_ACK) break;
            if (res.flag & RES_MALFORM) continue;
        }

        fprintf(stderr, "sent init for '%06d' (%d bytes)", file.filename, file.size);

        size_t offset = 0;
        std::set<size_t> remaining{};
        for(size_t i = 0; i * DATA_SIZE < file.size; i++)
            remaining.emplace(i+1);

        /*
        while (!remaining.empty()) {
            for(auto seq: remaining) {
                wrap_send(seq, connfd, file.data + (seq-1) * DATA_SIZE, DATA_SIZE);
            }
            wrap_send(seq, connfd, file.data + (seq-1) * DATA_SIZE, DATA_SIZE);
            response_hdr_t res;
            if (recv(connfd, &res, sizeof(res), 0) != sizeof(res))
                continue;
            if (res.flag_check ^ RES_MAGIC ^ res.flag)
                continue;
            if (res.flag & RES_ACK) continue;
            if (res.flag & RES_MALFORM) continue;
            if (res.flag & RES_FIN) break;
        }
        */
    }

}
