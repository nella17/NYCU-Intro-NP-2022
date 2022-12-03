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

void do_reponse(int sock, struct sockaddr_in* cin, sess_seq_u sess_seq, uint32_t flag) {
    response_hdr_t hdr{
        .sess_seq = sess_seq,
        .flag = flag,
    };
#ifdef USE_CHECKSUM
    hdr.checksum = checksum(&hdr);
#endif
    cin->sin_family = AF_INET;

    if (DEBUG) {
        printf("[*] Sending response to %s:%d, seq=%d, flag=%d\n", inet_ntoa(cin->sin_addr), ntohs(cin->sin_port), hdr.sess_seq.seq, hdr.flag);
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
        do_reponse(sock, cin, hdr->sess_seq, RES_MALFORM);
        return nullptr;
    }
#endif

    return hdr;
}

void save_to_file(char* filename, uint32_t comp_size, DATA_MAP_T& data) {
    int fp = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fp == -1) {
        fail("open");
    }

    // // dump map
    // for (auto it = data.begin(); it != data.end(); ++it) {
    //     printf("[Chunk %d] ", it->first);
    //     for (int i = 0; i < PACKET_SIZE; ++i) {
    //         printf("%02x ", (uint8_t)it->second[i]);
    //     }
    // }
    // printf("\n");

    auto comp_data = new char[(1 + (comp_size-1) / DATA_SIZE) * DATA_SIZE];
    for(auto [seq, data_frag]: data)
        if (seq)
            memcpy(comp_data + (seq-1) * DATA_SIZE, &data_frag, DATA_SIZE);

    auto orig_size = ZSTD_getFrameContentSize(comp_data, comp_size);
    auto orig_data = new char[orig_size];
    ZSTD_decompress(orig_data, orig_size, comp_data, comp_size);

    printf("[/] '%s' (%u bytes -> %llu bytes)\n", filename, comp_size, orig_size);
    write(fp, orig_data, orig_size);

    delete [] comp_data;
    delete [] orig_data;
    data.clear();
    close(fp);
}

int main(int argc, char *argv[]) {
    int s;
    struct sockaddr_in sin;
    int NUM_FILES = atoi(argv[2]);

    if(argc < 4) {
        return -fprintf(stderr, "usage: %s <path-to-store-files> <total-number-of-files> <port>\n", argv[0]);
    }

    setvbufs();

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons((uint16_t) strtol(argv[argc-1], NULL, 0));
    if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        fail("socket");

    if(bind(s, (struct sockaddr*) &sin, sizeof(sin)) < 0) {
        fail("bind");
    }

    std::map<uint16_t, session_t> session_map;
    std::map<uint16_t, DATA_MAP_T> data_map;
    int completed_files = 0;
    sender_hdr_t recv_hdr;
    while (true or completed_files != NUM_FILES) {
        struct sockaddr_in csin;
        bzero(&csin, sizeof(csin));
        std::map<uint32_t, char*> data;

        if (recv_sender_data(&recv_hdr, s, &csin) == nullptr)
            continue;

        // session initialization
        auto sess_id = recv_hdr.sess_seq.sess;
        auto data_seq = recv_hdr.sess_seq.seq;
        if (data_seq == 0) {
            // create a new session
            if (session_map.count(sess_id) == 0) {
                auto &session = session_map[sess_id];
                memcpy(&session.file_metadata, recv_hdr.data, sizeof(init_t));
                // printf("[/] [SessID:%d][ChunkID=0] Received connection initiation from %s:%d\n", sess_id, inet_ntoa(csin.sin_addr), ntohs(csin.sin_port));
                // printf("[/] [SessID:%d][ChunkID=0] filename: %d\n", sess_id, session->file_metadata.filename);
                // printf("[/] [SessID:%d][ChunkID=0] total size: %d\n", sess_id, session->file_metadata.filesize);
                printf("[/] [SessID:%d][ChunkID=0] initiation %d: %d\n", sess_id, session.file_metadata.filename, session.file_metadata.filesize);
            }

            // send a ACK to the client
            do_reponse(s, &csin, recv_hdr.sess_seq, RES_ACK);

            continue;
        }

        // if the session is not initialized, send a RST
        auto it = session_map.find(sess_id);
        if (it == session_map.end()) {
            printf("[/] [SessID:%d][ChunkID=%d] Session not initialized, sending RST\n", sess_id, data_seq);
            do_reponse(s, &csin, recv_hdr.sess_seq, RES_RST);

            continue;
        }

        // If the connection is already initiated, receive the data chunks
        auto& session = it->second;
        if (session.received_bytes < session.file_metadata.filesize) {
            // save the data chunk
            if (DEBUG) {
                printf("[/] [SessID:%d][ChunkID=%d] [FZ=%d/%d] Received data chunk from %s:%d\n",
                    sess_id,
                    data_seq,
                    session.received_bytes,
                    session.file_metadata.filesize,
                    inet_ntoa(csin.sin_addr),
                    ntohs(csin.sin_port));
            }

            // send a ACK to the client
            do_reponse(s, &csin, recv_hdr.sess_seq, RES_ACK);

            auto &mp = data_map[sess_id];
            if (data_seq && mp.find(data_seq) == mp.end()) {
                auto& data_frag = mp[data_seq];
                memcpy(&data_frag, recv_hdr.data, DATA_SIZE);
                session.received_bytes += DATA_SIZE;
            }
        }

        // check is the last received is the last chunk of data
        if (!session.is_complete && session.received_bytes >= session.file_metadata.filesize) {
            // All data recvived, send FIN
            printf("[/] [SessID:%d][ChunkID=%d] Received all data, sending FIN\n", sess_id, data_seq);
            sess_seq_u sess_seq{
                .sess = sess_id,
                .seq = 0
            };
            do_reponse(s, &csin, sess_seq, RES_FIN);

            init_t& file_metadata = session.file_metadata;
            char filename[100];
            sprintf(filename, "%s/%06d", argv[1], file_metadata.filename);
            printf("[/] [SessID:%d][ChunkID=%d] File saved to %s (%d | %lu)\n", sess_id, data_seq, filename, file_metadata.filesize, data_map[sess_id].size());
            save_to_file(filename, file_metadata.filesize, data_map[sess_id]);

            session.is_complete = true;
            completed_files++;
        } else if (session.is_complete) {
            sess_seq_u sess_seq{
                .sess = sess_id,
                .seq = 0
            };
            do_reponse(s, &csin, sess_seq, RES_FIN);
        }

    }
    close(s);
}
