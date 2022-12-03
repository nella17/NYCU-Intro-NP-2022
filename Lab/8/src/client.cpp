#include <cassert>
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
#include <map>
#include <utility>
#include <algorithm>

#include "header.hpp"
#include "util.hpp"

#include "zstd.h"

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

inline void wrap_send(uint32_t data_seq, int sockfd, const void* buf, size_t len) {
    sender_hdr_t hdr;
    bzero(&hdr, PACKET_SIZE);
    hdr.data_seq = data_seq;
    memcpy(hdr.data, buf, len);
#ifdef USE_CHECKSUM
    hdr.checksum = checksum(&hdr);
#endif
    // dump_hdr(&hdr);
    // for(int i = 0; i < 3; i++)
        send(sockfd, &hdr, PACKET_SIZE, MSG_DONTWAIT);
}

int main(int argc, char *argv[]) {
    if (argc != 5)
        return -fprintf(stderr, "Usage: %s <path-to-read-files> <total-number-of-files> <port> <server-ip-address>", argv[0]);

    setvbufs();
    // signal(SIGCHLD, SIG_IGN);
    // signal(SIGPIPE, SIG_IGN);

    char* path = argv[1];
    uint32_t total = (uint32_t)atoi(argv[2]);
    uint16_t port = (uint16_t)atoi(argv[3]);
    char* ip = argv[4];

    std::vector<file_t> files(total);

    std::map<uint32_t, data_t> data_map{};

    size_t total_bytes = 0;
    for(uint32_t idx = 0; idx < total; idx++) {
        char filename[256];
        sprintf(filename, "%s/%06d", path, idx);
        int filefd = open(filename, O_RDONLY);
        if (filefd < 0) fail("open");
        auto size = (uint32_t)lseek(filefd, 0, SEEK_END);
        auto data = new char[size];
        lseek(filefd, 0, SEEK_SET);
        read(filefd, data, size);
        close(filefd);

        files[idx] = {
            .filename = idx,
            .size = size,
            .init = {
                .filename = idx,
                .filesize = size,
            },
            .data = data,
        };
        printf("[cli] read '%s' (%u bytes)\n", filename, size);
        total_bytes += size;
    }

    auto orig_size = total * sizeof(init_t) + total_bytes;
    auto orig_data = new char[orig_size];
    {
        auto ptr = orig_data;
        for(auto& file: files) {
            memcpy(ptr, &file.init, sizeof(init_t));
            ptr += sizeof(init_t);
            memcpy(ptr, file.data, file.size);
            ptr += file.size;
        }
    }
    auto buf_size = ZSTD_compressBound(orig_size);
    auto comp_data = new char[buf_size];
    auto res = ZSTD_compress(comp_data, buf_size, orig_data, orig_size, COMPRESS_LEVEL);
    if (ZSTD_isError(res)) {
        fprintf(stderr, "%s\n", ZSTD_getErrorName(res));
        exit(-1);
    }
    auto comp_size = (uint32_t)res;

    printf("[cli] total %lu -> %u bytes\n", total_bytes, comp_size);

    int connfd = connect(ip, port);
    set_sockopt(connfd);

    {
        auto tmp = new data_t;
        tmp->data_size = comp_size;
        data_map.emplace(0, data_t { .data_size = sizeof(data_t), .data = (char*)tmp });
    }
    for(size_t i = 0; i * DATA_SIZE < comp_size; i++) {
        size_t offset = i * DATA_SIZE;
        data_map.emplace(
            i+1,
            data_t{
                .data_size = std::min(DATA_SIZE, comp_size - offset),
                .data = comp_data + offset,
            }
        );
    }

    std::vector<uint32_t> ackq{};
    auto read_resps = [&]() {
        response_hdr_t res;
        while (recv(connfd, &res, sizeof(res), MSG_WAITALL) == sizeof(res)) {
#ifdef USE_CHECKSUM
            if (checksum(&res) != res.checksum) {
                dump_hdr(&res);
                continue;
            }
#endif
            if (res.flag & RES_ACK) {
                ackq.emplace_back(res.data_seq);
                continue;
            }
            if (res.flag & RES_MALFORM) {
                // TODO
                continue;
            }
            if (res.flag & RES_FIN) {
                printf("[cli] sent done for (%u bytes)\n", comp_size);
                // usleep(100);
                exit(0);
                continue;
            }
            if (res.flag & RES_RST) {
                // TODO
                continue;
            }
        }
    };

    while (!data_map.empty()) {
        printf("[cli] %lu data packets left\n", data_map.size());
        size_t cnt = 0;
        for(auto [key, data] : data_map) {
            wrap_send(key, connfd, data.data, data.data_size);
            if (++cnt % WINDOW_SIZE == 0)
                read_resps();
        }
        read_resps();
        for(auto x: ackq)
            data_map.erase(x);
        ackq.clear();
    }

    return 0;
}
