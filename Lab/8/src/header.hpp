#pragma once
#include <cstdint>
#include <unistd.h>

constexpr uint32_t RES_ACK     = 1 << 0;
constexpr uint32_t RES_MALFORM = 1 << 1;
constexpr uint32_t RES_FIN     = 1 << 2;
constexpr uint32_t RES_RST     = 1 << 3;
constexpr uint32_t RES_MAGIC   = 0xFACEBEEF;

struct file_t {
    uint32_t filename;
    size_t   size;
    char*    data;
};

struct data_t {
    uint32_t filename;
    size_t data_size;
    char*    data;
};

struct init_t {
    uint32_t filename;
    uint32_t filesize;
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

constexpr size_t DATA_SIZE = 1400;
struct sender_hdr_t {
    sess_seq_u sess_seq;
    uint32_t checksum;
    char     data[DATA_SIZE];
};
constexpr size_t PACKET_SIZE = sizeof(sender_hdr_t);

struct response_hdr_t {
    sess_seq_u sess_seq;
    uint32_t checksum;
    uint32_t flag;
};
