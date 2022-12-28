#pragma once

#include <stdint.h>
#include <iostream>
#include <vector>

#include "record.hpp"

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define MSG_FLAG \
    struct __attribute__((packed)) { \
        unsigned RD:1, TC:1, AA:1, Opcode:4, QR:1; \
        unsigned RCODE:4, Z:3, RA:1; \
    }
#else
#define MSG_FLAG \
    struct __attribute__((packed)) { \
        unsigned QR:1, Opcode:4, AA:1, TC:1, RD:1; \
        unsigned RA:1, Z:3, RCODE:4; \
    }
#endif

struct Message {
    struct {
        uint16_t ID;
        MSG_FLAG;
        uint16_t QDCOUNT, ANCOUNT, NSCOUNT, ARCOUNT;
    };
    char buf[];
};

constexpr size_t MSG_SIZE = 12;

class Header {
public:
    union {
        char buf[MSG_SIZE];
        struct {
            uint16_t ID;
            MSG_FLAG;
            uint16_t QDCOUNT, ANCOUNT, NSCOUNT, ARCOUNT;
        };
    };
    unsigned RRcount;
    std::vector<Question> question;
    std::vector<Record> answer, authority, additional;
    std::vector<std::pair<uint16_t&,std::vector<Record>&>> ans;
    Header();
    void parse(const void*);
    std::string dump();
};

std::ostream& operator<<(std::ostream&, const Header&);
