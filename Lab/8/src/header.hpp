#pragma once
#include <cstdint>
#include <unistd.h>

constexpr uint32_t RES_ACK     = 1 << 0;
constexpr uint32_t RES_MALFORM = 1 << 1;
constexpr uint32_t RES_FIN     = 1 << 2;
constexpr uint32_t RES_MAGIC   = 0xFACEBEEF;


struct file_t {
    uint32_t filename;
    uint32_t size;
    char*    data;
};

struct init_t {
    uint32_t filename;
    uint32_t filesize;
};

constexpr size_t DATA_SIZE = 512;
struct sender_hdr_t {
    uint32_t data_seq;
    uint32_t checksum;
    char     data[DATA_SIZE];
};
constexpr size_t PACKET_SIZE = sizeof(sender_hdr_t);

struct response_hdr_t {
    uint32_t data_seq;
    uint32_t flag_check;
    uint32_t flag;
};
