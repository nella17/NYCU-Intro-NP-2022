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

inline void wrap_send(uint16_t sess_id, uint16_t seq, int sockfd, const void* buf, size_t len) {
    sender_hdr_t hdr;
    bzero(&hdr, PACKET_SIZE);
    hdr.sess_seq = {
        .sess = sess_id,
        .seq  = seq,
    };
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

    std::map<uint16_t, uint32_t> sess_ids{};
    std::map<sess_seq_u, data_t> data_map{};

    size_t total_bytes = 0;
    for(uint32_t idx = 0; idx < total; idx++) {
        char filename[256];
        sprintf(filename, "%s/%06d", path, idx);
        int filefd = open(filename, O_RDONLY);
        if (filefd < 0) fail("open");
        auto orig_size = (uint32_t)lseek(filefd, 0, SEEK_END);
        auto orig_data = new char[orig_size];
        lseek(filefd, 0, SEEK_SET);
        read(filefd, orig_data, orig_size);
        close(filefd);
        auto buf_size = ZSTD_compressBound(orig_size);
        auto comp_data = new char[buf_size];
        auto res = ZSTD_compress(comp_data, buf_size, orig_data, orig_size, COMPRESS_LEVEL);
        if (ZSTD_isError(res)) {
            fprintf(stderr, "%s\n", ZSTD_getErrorName(res));
            exit(-1);
        }
        auto comp_size = (uint32_t)res;

        files[idx] = {
            .filename = idx,
            .size = comp_size,
            .init = {
                .filename = idx,
                .filesize = comp_size,
            },
            .data = comp_data,
        };
        fprintf(stderr, "[cli] read '%s' (%u bytes -> %u bytes)\n", filename, orig_size, comp_size);
        total_bytes += comp_size;

        delete [] orig_data;
    }
    fprintf(stderr, "[cli] total %lu bytes\n", total_bytes);

    std::sort(files.begin(), files.end(), [&](auto a, auto b) { return a.size < b.size; });

    for(uint32_t idx = 0; idx < total; idx++) {
        auto &file = files[idx];
        uint16_t sess_id = (uint16_t)idx;
        sess_ids.emplace(sess_id, idx);
        data_map.emplace(
            sess_seq_u{
                .sess = sess_id,
                .seq = 0
            },
            data_t{
                .filename = file.filename,
                .data_size = sizeof(file.init),
                .data = (char*)&file.init,
            }
        );
    }

    int connfd = connect(ip, port);
    set_sockopt(connfd);

    auto wait_sess_ids(sess_ids);

    auto add_file_to_queue = [&](uint16_t sess_id) {
        auto it = sess_ids.find(sess_id);
        if (it == sess_ids.end()) return;
        uint32_t idx = it->second;
        auto &file = files[idx];
        for(size_t i = 0; i * DATA_SIZE < file.size; i++) {
            size_t offset = i * DATA_SIZE;
            data_map.emplace(
                sess_seq_u{
                    .sess = sess_id,
                    .seq  = uint16_t(i+1),
                },
                data_t{
                    .filename = idx,
                    .data_size = std::min(DATA_SIZE, file.size - offset),
                    .data = file.data + offset
                }
            );
        }
    };

    auto erase_file_from_queue = [&](uint16_t sess_id) {
        auto it = sess_ids.find(sess_id);
        if (it == sess_ids.end()) return;
        uint32_t idx = it->second;
        auto &file = files[idx];
        for(size_t i = 0; i * DATA_SIZE < file.size; i++) {
            data_map.erase(sess_seq_u{
                .sess = sess_id,
                .seq  = uint16_t(i+1),
            });
        }
    };

    std::vector<sess_seq_u> ackq{};
    auto read_resps = [&]() {
        response_hdr_t res;
        while (recv(connfd, &res, sizeof(res), 0) == sizeof(res)) {
#ifdef USE_CHECKSUM
            if (checksum(&res) != res.checksum) {
                dump_hdr(&res);
                continue;
            }
#endif
            auto sess_id = res.sess_seq.sess;
            if (res.flag & RES_ACK) {
                ackq.emplace_back(res.sess_seq);
                if (res.sess_seq.seq == 0 and wait_sess_ids.count(sess_id)) {
                    wait_sess_ids.erase(sess_id);
                    fprintf(stderr, "[cli] sess_id %u acked\n", sess_id);
                    add_file_to_queue(sess_id);
                }
                continue;
            }
            if (res.flag & RES_MALFORM) {
                // TODO
                continue;
            }
            if (res.flag & RES_FIN) {
                auto idx = sess_ids[res.sess_seq.sess];
                auto &file = files[idx];
                fprintf(stderr, "[cli] sent done for '%06d' (%lu bytes)\n", file.filename, file.size);
                erase_file_from_queue(sess_id);
                continue;
            }
            if (res.flag & RES_RST) {
                // TODO
                continue;
            }
        }
    };

    while (!data_map.empty() || !wait_sess_ids.empty()) {
        fprintf(stderr, "[cli] %lu data packets left\n", data_map.size());
        if (!wait_sess_ids.empty())
            fprintf(stderr, "[cli] %lu sess_id left\n", wait_sess_ids.size());

        size_t cnt = 0;
        for(auto [key, data] : data_map) {
            wrap_send(key.sess, key.seq, connfd, data.data, data.data_size);
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
