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


void do_reponse(int sock, struct sockaddr_in* cin, struct response_hdr_t* hdr) {
    hdr->flag_check = hdr->flag ^ RES_MAGIC;

    cin->sin_family = AF_INET;

    printf("[*] Sending response to %s:%d, seq=%d, flag=%d\n", inet_ntoa(cin->sin_addr), ntohs(cin->sin_port), hdr->data_seq, hdr->flag);
    for (int i = 0; i < 10; ++i) {
        if(sendto(sock, (const void *) hdr, sizeof(struct response_hdr_t), 0, (struct sockaddr*) cin, sizeof(sockaddr_in)) < 0) {
            fail("sendto");
        }
    }
}

void reponse_malformed(int sock, uint32_t seq, struct sockaddr_in* cin) {
    struct response_hdr_t hdr;
    hdr.data_seq = seq;
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
    if (buf->checksum != adler32(buf->data, DATA_SIZE)) {
        dump_sender_hdr((sender_hdr_t*) buf);
        reponse_malformed(sock, buf->data_seq, cin);
        return NULL;
    }

    return (sender_hdr_t*) buf;
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

    for (int file_id = 0; file_id < NUM_FILES; ++file_id) {
        struct sockaddr_in csin;
        bzero(&csin, sizeof(csin));
        bool is_init = 0;
        std::map<uint32_t, char*> data;

        // connection initiation
        response_hdr_t response;
        init_t file_metadata;
        while (!is_init) {
            sender_hdr_t* recv_hdr = recv_sender_data(s, &csin);

            if (recv_hdr == NULL) {
                continue;
            }

            if (recv_hdr->data_seq != 0) {
                reponse_malformed(s, 0, &csin);
                continue;
            }

            memcpy(&file_metadata, recv_hdr->data, sizeof(init_t));

            printf("[/] [FN:%d][DN=0] Received connection initiation from %s:%d\n", file_id, inet_ntoa(csin.sin_addr), ntohs(csin.sin_port));
            printf("[/] [FN:%d][DN=0] filename: %d\n", file_id, file_metadata.filename);
            printf("[/] [FN:%d][DN=0] total size: %d\n", file_id, file_metadata.filesize);

            // send a ACK to the client
            response.data_seq = 0;
            response.flag = RES_ACK;
            do_reponse(s, &csin, &response);

            is_init = 1;
            free(recv_hdr);
        }

        // TODO: recv file
        u_int32_t recved_size = 0;
        while (recved_size < file_metadata.filesize) {
            sender_hdr_t* recv_hdr = recv_sender_data(s, &csin);

            if (recv_hdr == NULL) {
                continue;
            }

            printf("[/] [FN:%d][DN=%d] Received data from %s:%d\n", file_id, recv_hdr->data_seq, inet_ntoa(csin.sin_addr), ntohs(csin.sin_port));

            // send a ACK to the client
            response.data_seq = recv_hdr->data_seq;
            response.flag = RES_ACK;
            do_reponse(s, &csin, &response);

            char* data_frag = (char*) malloc(DATA_SIZE);
            memcpy(data_frag, recv_hdr->data, DATA_SIZE);
            if (recv_hdr->data_seq && data.find(recv_hdr->data_seq) == data.end()) {
                data[recv_hdr->data_seq] = data_frag;
                recved_size += DATA_SIZE;
            }

            free(recv_hdr);
        }

        // All data recvived, send FIN
        response.data_seq = 0;
        response.flag = RES_FIN;
        do_reponse(s, &csin, &response);

        // dump map
        printf("[/] [FN:%d] Dumping data map\n", file_id);
        for (auto it = data.begin(); it != data.end(); ++it) {
            printf("[/] [FN:%d][DN=%d] ", file_id, it->first);
            for (size_t i = 0; i < DATA_SIZE; ++i) {
                printf("%02x", ((uint8_t*)it->second)[i]);
            }
            printf("\n");
        }
        printf("\n");

        // truncate the file to the right size
        char* filename = (char*) malloc(100);
        size_t last_block_size = file_metadata.filesize % DATA_SIZE;
        last_block_size = (last_block_size == 0) ? DATA_SIZE : last_block_size;

        sprintf(filename, "%s/%06d", argv[1], file_metadata.filename);
        int fp = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
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
        close(fp);
    }
    close(s);
}
