#pragma once

#include <stdint.h>
#include <iostream>
#include <vector>

#include "record.hpp"

struct Message {
    struct {
        uint16_t ID;
        struct __attribute__((packed)) {
            unsigned QR:1, Opcode:4, AA:1, TC:1, RD:1, RA:1, Z:3, RCODE:4;
        };
        uint16_t QDCOUNT, ANCOUNT, NSCOUNT, ARCOUNT;
    };
    char buf[];
};

constexpr size_t MSG_SIZE = 12;

class Header {
public:
    Message* msg;
    union {
        char buf[MSG_SIZE];
        struct {
            uint16_t ID;
            struct __attribute__((packed)) {
                unsigned QR:1, Opcode:4, AA:1, TC:1, RD:1, RA:1, Z:3, RCODE:4;
            };
            uint16_t QDCOUNT, ANCOUNT, NSCOUNT, ARCOUNT;
        };
    };
    unsigned RRcount;
    std::vector<Question> question;
    std::vector<Record> answer, authority, additional;
    std::vector<std::pair<uint16_t&,std::vector<Record>&>> ans;
    Header();
    void parse(void*);
    void* dump(bool = true);
};

std::ostream& operator<<(std::ostream&, const Header&);
