#pragma once
#include <cstdint>
#include <unistd.h>

#ifndef DEBUG
#define printf x
#define fprintf x
int x(...) { return 1; }
#endif
// #define USE_CHECKSUM

constexpr uint32_t RES_ACK     = 1 << 0;
constexpr uint32_t RES_MALFORM = 1 << 1;
constexpr uint32_t RES_FIN     = 1 << 2;
constexpr uint32_t RES_RST     = 1 << 3;
constexpr uint32_t RES_MAGIC   = 0xFACEBEEF;

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

struct data_t {
    size_t   data_size;
    char*    data;
};

constexpr size_t HEADER_SIZE = 28;
constexpr size_t DATA_SIZE = 1500 - HEADER_SIZE - sizeof(uint32_t)
#ifdef USE_CHECKSUM
    - sizeof(uint32_t);
#else
    ;
#endif
struct sender_hdr_t {
    uint32_t data_seq = 0;
#ifdef USE_CHECKSUM
    uint32_t checksum = 0;
#endif
    char     data[DATA_SIZE] = { 0 };
};
constexpr size_t PACKET_SIZE = sizeof(sender_hdr_t);
constexpr size_t TOTAL_SIZE = PACKET_SIZE + HEADER_SIZE;

constexpr size_t BANDWIDTH = 10 * 1024 * 1024 / 8;
constexpr size_t WINDOW_SIZE = BANDWIDTH / (TOTAL_SIZE << 4);

struct response_hdr_t {
    uint32_t data_seq;
#ifdef USE_CHECKSUM
    uint32_t checksum;
#endif
    uint32_t flag;
};
