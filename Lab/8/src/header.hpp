#pragma once
#include <cstdint>
#include <unistd.h>

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
    uint32_t filename;
    size_t   data_size;
    char*    data;
};

union sess_seq_u {
    uint32_t id;
    struct {
        uint16_t sess;
        uint16_t seq;
    };
    bool operator<(const sess_seq_u &rhs) const {
        return id < rhs.id;
    }

};

constexpr size_t HEADER_SIZE = 28;
constexpr size_t DATA_SIZE = 1400;
struct sender_hdr_t {
    sess_seq_u sess_seq = { .id = 0 };
    uint32_t checksum = 0;
    char     data[DATA_SIZE] = { 0 };
};
constexpr size_t PACKET_SIZE = sizeof(sender_hdr_t);
constexpr size_t TOTAL_SIZE = PACKET_SIZE + HEADER_SIZE;

constexpr size_t BANDWIDTH = 10 * 1024 * 1024 / 8 / 16;
constexpr size_t WINDOW_SIZE = BANDWIDTH / TOTAL_SIZE;

struct response_hdr_t {
    sess_seq_u sess_seq;
    uint32_t checksum;
    uint32_t flag;
};
