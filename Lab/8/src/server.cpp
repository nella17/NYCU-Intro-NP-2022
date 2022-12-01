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


struct session_t {
    init_t file_metadata;
    uint32_t received_bytes = 0;
    bool is_complete = false;
};

void do_reponse(int sock, struct sockaddr_in* cin, struct response_hdr_t* hdr) {
    hdr->flag_check = hdr->flag ^ RES_MAGIC;
    cin->sin_family = AF_INET;

    if (DEBUG) {
        printf("[*] Sending response to %s:%d, seq=%d, flag=%d\n", inet_ntoa(cin->sin_addr), ntohs(cin->sin_port), hdr->data_seq, hdr->flag);
    }
    for (int i = 0; i < 4; ++i) {
        if(sendto(sock, (const void *) hdr, sizeof(struct response_hdr_t), 0, (struct sockaddr*) cin, sizeof(sockaddr_in)) < 0) {
            fail("sendto");
        }
    }
}

void reponse_malformed(int sock, uint16_t sess_id, uint16_t seq_id, struct sockaddr_in* cin) {
    struct response_hdr_t hdr;
    hdr.sess_id = sess_id;
    hdr.data_seq = seq_id;
    hdr.flag = RES_MALFORM;
    hdr.flag_check = hdr.flag ^ RES_MAGIC;
    do_reponse(sock, cin, &hdr);
}

/*
 * Receive sender data and check if the packet is valid
 *
 * @return sender_hdr_t* if the packet is valid, NULL otherwise
 */
sender_hdr_t* recv_sender_data(int sock, struct sockaddr_in* cin) {
    sender_hdr_t* buf = (sender_hdr_t*) malloc(PACKET_SIZE);
    bzero(buf, PACKET_SIZE);
    socklen_t len = sizeof(&cin);

    if(recvfrom(sock, (void *) buf, PACKET_SIZE, 0, (struct sockaddr*) cin, &len) < 0) {
        fail("recvfrom");
    }

    // perform checksum
    if (buf->checksum != checksum(buf)) {
        dump_sender_hdr((sender_hdr_t*) buf);
        reponse_malformed(sock, buf->sess_id, buf->data_seq, cin);
        return NULL;
    }

    return (sender_hdr_t*) buf;
}

void save_to_file(char* filename, uint32_t filesize, std::map<uint32_t, char*> data) {
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

    size_t last_block_size = filesize % DATA_SIZE;
    if (last_block_size == 0) {
        last_block_size = DATA_SIZE;
    }
    for(auto it = data.begin(); it != data.end(); it++) {
        auto data_frag = it->second;
        if (it->first) {
            if (next(it) == data.end()) {  // last chunk of data
                write(fp, data_frag, last_block_size);
            } else {
                write(fp, data_frag, DATA_SIZE);
            }
        }
        free(data_frag);
    }

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

    std::map<uint16_t, session_t*> session_map;
    std::map<uint16_t, std::map<uint32_t, char*>> data_map;
    int completed_files = 0;
    while (completed_files != NUM_FILES) {
        struct sockaddr_in csin;
        bzero(&csin, sizeof(csin));
        std::map<uint32_t, char*> data;
        response_hdr_t response;

        sender_hdr_t* recv_hdr = recv_sender_data(s, &csin);
        if (recv_hdr == NULL) {
            continue;
        }

        // session initialization
        if (recv_hdr->data_seq == 0) {
            // create a new session
            session_t* session = (session_t*) malloc(sizeof(session_t));
            memcpy(&session->file_metadata, recv_hdr->data, sizeof(init_t));
            session_map[recv_hdr->sess_id] = session;

            printf("[/] [SessID:%d][ChunkID=0] Received connection initiation from %s:%d\n", recv_hdr->sess_id, inet_ntoa(csin.sin_addr), ntohs(csin.sin_port));
            printf("[/] [SessID:%d][ChunkID=0] filename: %d\n", recv_hdr->sess_id, session->file_metadata.filename);
            printf("[/] [SessID:%d][ChunkID=0] total size: %d\n", recv_hdr->sess_id, session->file_metadata.filesize);

            // send a ACK to the client
            response.sess_id = recv_hdr->sess_id;
            response.data_seq = 0;
            response.flag = RES_ACK;
            do_reponse(s, &csin, &response);

            free(recv_hdr);
            continue;
        }

        // if the session is not initialized, send a RST
        if (session_map.find(recv_hdr->sess_id) == session_map.end()) {
            printf("[/] [SessID:%d][ChunkID=%d] Session not initialized, sending RST\n", recv_hdr->sess_id, recv_hdr->data_seq);
            response.sess_id = recv_hdr->sess_id;
            response.data_seq = recv_hdr->data_seq;
            response.flag = RES_RST;
            do_reponse(s, &csin, &response);

            free(recv_hdr);
            continue;
        }

        // If the connection is already initiated, receive the data chunks
        uint16_t sess_id = recv_hdr->sess_id;
        auto session = session_map[sess_id];
        if (session->received_bytes < session->file_metadata.filesize) {
            // save the data chunk
            if (DEBUG) {
                printf("[/] [SessID:%d][ChunkID=%d] [FZ=%d/%d] Received data chunk from %s:%d\n",
                    recv_hdr->sess_id,
                    recv_hdr->data_seq,
                    session->received_bytes,
                    session->file_metadata.filesize,
                    inet_ntoa(csin.sin_addr),
                    ntohs(csin.sin_port));
            }

            // send a ACK to the client
            response.sess_id = recv_hdr->sess_id;
            response.data_seq = recv_hdr->data_seq;
            response.flag = RES_ACK;
            do_reponse(s, &csin, &response);

            auto &mp = data_map[recv_hdr->sess_id];
            if (recv_hdr->data_seq && mp.find(recv_hdr->data_seq) == mp.end()) {
                char* data_frag = (char*) malloc(DATA_SIZE);
                memcpy(data_frag, recv_hdr->data, DATA_SIZE);
                mp[recv_hdr->data_seq] = data_frag;
                session->received_bytes += DATA_SIZE;
            }
        }

        // check is the last received is the last chunk of data
        if (!session->is_complete && session->received_bytes >= session->file_metadata.filesize) {
            // All data recvived, send FIN
            printf("[/] [SessID:%d][ChunkID=%d] Received all data, sending FIN\n", recv_hdr->sess_id, recv_hdr->data_seq);
            response.sess_id = recv_hdr->sess_id;
            response.data_seq = 0;
            response.flag = RES_FIN;
            do_reponse(s, &csin, &response);

            init_t* file_metadata = &session->file_metadata;
            char* filename = (char*) malloc(100);
            sprintf(filename, "%s/%06d", argv[1], file_metadata->filename);
            printf("[/] [SessID:%d][ChunkID=%d] File saved to %s\n", recv_hdr->sess_id, recv_hdr->data_seq, filename);
            save_to_file(filename, file_metadata->filesize, data_map[sess_id]);

            session->is_complete = true;
            completed_files++;
            free(filename);
        } else if (session->is_complete) {
            response.sess_id = recv_hdr->sess_id;
            response.data_seq = 0;
            response.flag = RES_FIN;
            do_reponse(s, &csin, &response);
        }

        free(recv_hdr);
    }
    close(s);
}
