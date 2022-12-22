#pragma once

#include <limits.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <chrono>
#include <bitset>
#include <vector>
#include <set>
#include <map>
#include <utility>
#include <algorithm>
#include <random>
#include <thread>

constexpr int TTL_OUT = 64;
constexpr int IPPROTO_XDP = 153;
constexpr uint32_t MOD_ADLER = int(1e9)+7;

constexpr size_t PACKET_SIZE = 1500;

struct init_t {
    uint32_t filename;
    uint32_t filesize;
};

struct file_t {
    uint32_t filename;
    size_t   size;
    init_t   init;
    char*    data;
};

struct data_t  {
    size_t   data_size;
    char*    data;
};

struct __attribute__((packed)) hdr_t {
    struct ip ip_hdr;
    union {
        uint32_t sess_seq;
        struct {
            uint16_t sess_id, seq_id;
        };
    };
    uint32_t checksum;
};

constexpr size_t HEADER_SIZE = sizeof(hdr_t);
constexpr size_t DATA_SIZE = PACKET_SIZE - HEADER_SIZE - sizeof(uint32_t) - sizeof(uint32_t);

struct __attribute__((packed)) pkt_t {
    hdr_t    header;
    char     data[DATA_SIZE] = { 0 };
};

inline void fail(const char* s) {
    if (errno) perror(s);
    else fprintf(stderr, "%s: unknown error", s);
#ifndef DEBUG
    exit(-1);
#endif
}

inline void setup() {
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
}

inline uint32_t adler32(const void *_data, size_t len)
/*
    where data is the location of the data in physical memory and
    len is the length of the data in bytes
*/
{
    auto data = (const uint8_t*)_data;
    uint32_t a = 1, b = 0;
    size_t index;

    // Process each byte of the data in order
    for (index = 0; index < len; ++index)
    {
        a = (a + data[index]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }

    return (b << 16) | a;
}

inline uint32_t checksum(const pkt_t* packet) {
    uint32_t value = adler32(packet->data, DATA_SIZE);
    return value ^ packet->header.sess_seq;
}

inline int create_sock(int c = 0) {
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_XDP);
    if (sockfd < 0) fail("sockfd");

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        fail("setsockopt(SO_RCVTIMEO)");

    int on = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0)
        fail("setsockopt(SO_BROADCAST)");
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
        fail("setsockopt(IP_HDRINCL)");

    return sockfd;
}

inline unsigned short cksum(void *in, int sz) {
    long sum = 0;
    unsigned short *ptr = (unsigned short*) in;
    for(; sz > 1; sz -= 2) sum += *ptr++;
    if(sz > 0) sum += *((unsigned char*) ptr);
    while(sum>>16) sum = (sum & 0xffff) + (sum>>16);
    return ~sum;
}

long send(int sockfd, pkt_t pkt, struct sockaddr_in dst) {
    pkt.header.checksum = checksum(&pkt);
    auto ip = &pkt.header.ip_hdr;
    ip->ip_v = IPVERSION;
    ip->ip_hl = sizeof(struct ip) >> 2;
    ip->ip_tos = 0;
    ip->ip_len = htons(sizeof(pkt_t));
    ip->ip_id = 0;
    ip->ip_off = 0;
    ip->ip_ttl = TTL_OUT;
    ip->ip_p   = IPPROTO_XDP;
    ip->ip_src = dst.sin_addr;
    ip->ip_dst = dst.sin_addr;
    ip->ip_sum = cksum(ip, sizeof(struct ip));
    return sendto(sockfd, &pkt, sizeof(pkt), 0, (sockaddr*)&dst, sizeof(dst));
}
