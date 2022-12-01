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

inline void wrap_send(uint16_t sess_id, uint16_t seq, int sockfd, const void* buf, size_t len) {
    sender_hdr_t hdr;
    bzero(&hdr, PACKET_SIZE);
    hdr.sess_id = sess_id;
    hdr.data_seq = seq;
    memcpy(hdr.data, buf, len);
    hdr.checksum = checksum(&hdr);
    // dump_sender_hdr(&hdr);
    // for(int i = 0; i < 3; i++)
        send(sockfd, &hdr, PACKET_SIZE, MSG_DONTWAIT);
}

int main(int argc, char *argv[]) {
    if (argc != 5)
        return -fprintf(stderr, "Usage: %s <path-to-read-files> <total-number-of-files> <port> <server-ip-address>", argv[0]);

    setvbufs();
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    char* path = argv[1];
    uint32_t total = (uint32_t)atoi(argv[2]);
    uint16_t port = (uint16_t)atoi(argv[3]);
    char* ip = argv[4];

    std::vector<file_t> files(total);

    std::map<uint16_t, uint32_t> sess_ids{};
    std::map<std::pair<uint16_t, uint16_t>, data_t> data_map{};

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
        fprintf(stderr, "[cli] read '%s' (%lu bytes)\n", filename, file.size);

        uint16_t sess_id = (uint16_t)idx;
        sess_ids.emplace(sess_id, idx);
        for(size_t i = 0; i * DATA_SIZE < file.size; i++) {
            size_t offset = i * DATA_SIZE;
            data_map[{ sess_id , i+1 }] = {
                .filename = idx,
                .data_size = std::min(DATA_SIZE, file.size - offset),
                .data = file.data + offset
            };
        }
    }

    int connfd = connect(ip, port);
    set_sock_timeout(connfd);

    auto wait_sess_ids(sess_ids);
    while (!wait_sess_ids.empty()) {
        fprintf(stderr, "[cli] %lu sess_id left\n", wait_sess_ids.size());
        for (auto [sess_id, idx] : wait_sess_ids) {
            auto &file = files[idx];
            init_t init {
                .filename = file.filename,
                .filesize = (uint32_t)file.size
            };
            wrap_send(sess_id, 0, connfd, &init, sizeof(init));
        }
        response_hdr_t res;
        while (recv(connfd, &res, sizeof(res), 0) == sizeof(res)) {
            if (res.flag_check ^ RES_MAGIC ^ res.flag)
                continue;
            if (res.flag & RES_ACK) {
                if (res.data_seq == 0 and wait_sess_ids.count(res.sess_id)) {
                    wait_sess_ids.erase(res.sess_id);
                    fprintf(stderr, "[cli] sess_id %u acked\n", res.sess_id);
                }
                continue;
            }
            if (res.flag & RES_MALFORM) continue;
        }
    }

    while (!data_map.empty()) {
        fprintf(stderr, "[cli] %lu data packets left\n", data_map.size());
        for(auto [key, data] : data_map) {
            wrap_send(key.first, key.second, connfd, data.data, data.data_size);
        }

        response_hdr_t res;
        while (recv(connfd, &res, sizeof(res), 0) == sizeof(res)) {
            if (res.flag_check ^ RES_MAGIC ^ res.flag)
                continue;
            if (res.flag & RES_ACK) {
                data_map.erase({ res.sess_id, res.data_seq });
                continue;
            }
            if (res.flag & RES_MALFORM) {
                continue;
            }
            if (res.flag & RES_FIN) {
                auto idx = sess_ids[res.sess_id];
                auto &file = files[idx];
                fprintf(stderr, "[cli] sent done for '%06d' (%lu bytes)\n", file.filename, file.size);
                for(size_t i = 0; i * DATA_SIZE < file.size; i++)
                    data_map.erase({ res.sess_id , i+1 });
                continue;
            }
            /*
            if (res.flag & RES_RST) {
                auto idx = sess_ids[res.sess_id];
                auto &file = files[idx];
                fprintf(stderr, "[cli] ???? recv RET for '%06d' (%lu bytes)\n", file.filename, file.size);
                for(size_t i = 0; i * DATA_SIZE < file.size; i++)
                    data_map.erase({ res.sess_id , i+1 });
                continue;
            }
            */
        }
    }

}
