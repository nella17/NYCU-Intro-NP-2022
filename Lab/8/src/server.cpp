#include <array>
#include <arpa/inet.h>
#include <cstddef>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <filesystem>

#include "header.hpp"
#include "util.hpp"

#include "zstd.h"

struct session_t {
    init_t file_metadata;
    uint32_t received_bytes = 0;
    bool is_complete = false;
};

using DATA_MAP_T = std::map<uint32_t, std::array<char,DATA_SIZE>>;

void do_reponse(int sock, struct sockaddr_in* cin, uint32_t data_seq, uint32_t flag) {
    response_hdr_t hdr{
        .data_seq = data_seq,
        .flag = flag,
    };
#ifdef USE_CHECKSUM
    hdr.checksum = checksum(&hdr);
#endif
    cin->sin_family = AF_INET;

    if (DEBUG) {
        printf("[*] Sending response to %s:%d, seq=%d, flag=%d\n", inet_ntoa(cin->sin_addr), ntohs(cin->sin_port), hdr.data_seq, hdr.flag);
    }
    for (int i = 0; i < 2; ++i) {
        if(sendto(sock, (const void *) &hdr, sizeof(struct response_hdr_t), 0, (struct sockaddr*) cin, sizeof(sockaddr_in)) < 0) {
            fail("sendto");
        }
    }
}

/*
 * Receive sender data and check if the packet is valid
 *
 * @return sender_hdr_t* if the packet is valid, nullptr otherwise
 */
sender_hdr_t* recv_sender_data(sender_hdr_t* hdr, int sock, struct sockaddr_in* cin) {
    bzero(hdr, sizeof(sender_hdr_t));
    socklen_t len = sizeof(&cin);

    if(recvfrom(sock, (void *) hdr, PACKET_SIZE, MSG_WAITALL, (struct sockaddr*) cin, &len) < 0) {
        fail("recvfrom");
    }

    // perform checksum
#ifdef USE_CHECKSUM
    if (hdr->checksum != checksum(hdr)) {
        dump_hdr(hdr);
        do_reponse(sock, cin, hdr->data_seq, RES_MALFORM);
        return nullptr;
    }
#endif

    return hdr;
}

int save_to_file(const char* path, uint32_t comp_size, DATA_MAP_T& data) {
    auto comp_data = new char[(1 + (comp_size-1) / DATA_SIZE) * DATA_SIZE];
    for(auto [seq, data_frag]: data)
        if (seq)
            memcpy(comp_data + (seq-1) * DATA_SIZE, &data_frag, DATA_SIZE);

    auto orig_size = ZSTD_getFrameContentSize(comp_data, comp_size);
    auto orig_data = new char[orig_size];
    ZSTD_decompress(orig_data, orig_size, comp_data, comp_size);

    printf("[/] %u bytes -> %llu bytes\n", comp_size, orig_size);

    // // dump map
    // for (auto it = data.begin(); it != data.end(); ++it) {
    //     printf("[Chunk %d] ", it->first);
    //     for (int i = 0; i < PACKET_SIZE; ++i) {
    //         printf("%02x ", (uint8_t)it->second[i]);
    //     }
    // }
    // printf("\n");

    int cnt = 0;
    for(auto ptr = orig_data; ptr < orig_data + orig_size; cnt++) {
        auto init = (init_t*)ptr;
        ptr += sizeof(init_t);
        auto filesize = init->filesize;

        char filename[100];
        sprintf(filename, "%s/%06d", path, init->filename);
        printf("[/] File saved to %s (%d bytes)\n", filename, filesize);
        int fp = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fp == -1) fail("open");
        write(fp, ptr, filesize);
        ptr += filesize;
        close(fp);
    }

    delete [] comp_data;
    delete [] orig_data;
    data.clear();
    return cnt;
}

int main(int argc, char *argv[]) {
    if(argc < 4) {
        return -fprintf(stderr, "usage: %s <path-to-store-files> <total-number-of-files> <port>\n", argv[0]);
    }

    char* path = argv[1];
    int total = atoi(argv[2]);
    auto port = (uint16_t)atoi(argv[3]);

    setvbufs();

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    int listenfd;
    if((listenfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        fail("socket");

    if(bind(listenfd, (struct sockaddr*) &sin, sizeof(sin)) < 0) {
        fail("bind");
    }

    uint32_t comp_size = UINT_MAX, recv_size = 0;
    DATA_MAP_T data_map;
    sender_hdr_t recv_hdr;
    struct sockaddr_in csin;

    while (comp_size == UINT_MAX or recv_size < comp_size) {
        bzero(&csin, sizeof(csin));
        if (recv_sender_data(&recv_hdr, listenfd, &csin) == nullptr)
            continue;

        // session initialization
        auto data_seq = recv_hdr.data_seq;
        if (data_seq == 0) {
            // create a new session
            data_t tmp;
            memcpy(&tmp, recv_hdr.data, sizeof(data_t));
            comp_size = (uint32_t)tmp.data_size;
            printf("[/] [ChunkID=%d] initiation %d bytes\n", data_seq, comp_size);
        } else {
            // save the data chunk
            if (DEBUG) {
                printf("[/] [ChunkID=%d] Received data chunk from %s:%d\n",
                    data_seq,
                    inet_ntoa(csin.sin_addr),
                    ntohs(csin.sin_port)
                );
            }
            if (!data_map.count(data_seq)) {
                auto& data_frag = data_map[data_seq];
                memcpy(&data_frag, recv_hdr.data, DATA_SIZE);
                recv_size += DATA_SIZE;
            }
        }

        // send a ACK to the client
        do_reponse(listenfd, &csin, recv_hdr.data_seq, RES_ACK);
    }

    auto write = save_to_file(path, comp_size, data_map);
    printf("[/] save %d / %d files\n", write, total);

    // All data recvived, send FIN
    printf("[/] Received all data, sending FIN\n");
    while (true) {
        if (recv_sender_data(&recv_hdr, listenfd, &csin) == nullptr)
            continue;
        do_reponse(listenfd, &csin, 0, RES_FIN);
    }

    close(listenfd);

    return 0;
}
