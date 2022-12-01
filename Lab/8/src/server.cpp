#include <arpa/inet.h>
#include <cstddef>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    if(sendto(sock, (const void *) hdr, sizeof(struct response_hdr_t), 0, (struct sockaddr*) cin, sizeof(sockaddr_in)) < 0) {
        fail("sendto");
    }
}

void reponse_malformed(int sock, uint32_t seq, struct sockaddr_in* cin) {
    struct response_hdr_t hdr;
    hdr.data_seq = seq;
    hdr.flag = RES_MALFORM;
    hdr.flag_check = hdr.flag ^ RES_MAGIC;
    do_reponse(sock, cin, &hdr);
}

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

    char* buf = (char*) malloc(2048);
	for (int i = 0; i < NUM_FILES; ++i) {
		struct sockaddr_in csin;
        bzero(&csin, sizeof(csin));
        bool is_init = 0;
        std::map<int, char*> data;

        // connection initiation
        init_t* file_metadata;
        while (!is_init) {
            sender_hdr_t* recv_hdr = recv_sender_data(s, &csin);

            if (recv_hdr == NULL) {
                reponse_malformed(s, 0, &csin);
                continue;
            }

            if (recv_hdr->data_seq != 0) {
                reponse_malformed(s, 0, &csin);
                continue;
            }

            file_metadata = (init_t*) recv_hdr->data;
            printf("[/] [0] Received connection initiation from %s:%d\n", inet_ntoa(csin.sin_addr), ntohs(csin.sin_port));
            printf("[/] [0] filename: %d\n", file_metadata->filename);
            printf("[/] [0] total size: %d\n", file_metadata->filesize);

            // send a ACK to the client
            response_hdr_t* response = (response_hdr_t*) malloc(sizeof(response_hdr_t));
            response->data_seq = 0;
            response->flag = RES_ACK;
            do_reponse(s, &csin, response);

            is_init = 1;
            free(recv_hdr);
            free(response);
        }

        // TODO: recv file
	}

	close(s);
}
