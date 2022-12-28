#include <string.h>
#include <arpa/inet.h>

#include "header.hpp"
#include "utils.hpp"

Header::Header() {
    ans.reserve(3);
    ans.emplace_back(ANCOUNT, answer);
    ans.emplace_back(NSCOUNT, authority);
    ans.emplace_back(ARCOUNT, additional);
}

inline uint16_t data2uint16(char*& data) {
    auto r = *(uint16_t*)data;
    data += sizeof(r);
    return ntohs(r);
}
inline uint32_t data2uint32(char*& data) {
    auto r = *(uint32_t*)data;
    data += sizeof(r);
    return ntohl(r);
}

void Header::parse(void* _msg) {
    msg = (Message*)_msg;
    memcpy(buf, msg, MSG_SIZE);
    ID      = ntohs(msg->ID);
    // TODO: Opcode = ntoh(msg->Opcode);
    QDCOUNT = ntohs(msg->QDCOUNT);
    ANCOUNT = ntohs(msg->ANCOUNT);
    NSCOUNT = ntohs(msg->NSCOUNT);
    ARCOUNT = ntohs(msg->ARCOUNT);
    RRcount = ANCOUNT + NSCOUNT + ARCOUNT;
    auto ptr = msg->buf;
    question.clear();
    for(size_t i = 0; i < QDCOUNT; i++) {
        auto domain = data2dn(ptr);
        auto type = v2type(data2uint16(ptr));
        auto clas = v2clas(data2uint16(ptr));
        question.emplace_back(domain, type, clas);
    }
    for(auto& [sz,v]: ans) {
        v.clear();
        /* TODO: parse RR
        std::cout << sz << std::endl;
        for(size_t i = 0; i < sz; i++) {
            auto domain = data2dn(ptr);
            auto type = v2type(data2uint16(ptr));
            auto clas = v2clas(data2uint16(ptr));
            auto ttl = data2uint32(ptr);
            auto rdatalen = data2uint16(ptr);
            auto rdata = std::string(ptr, rdatalen);
            ptr += rdatalen;
            // v.emplace_back(domain, type, clas, ttl, rdatalen, rdata);
        }
        */
    }
}

void* Header::dump(bool create) {
    std::string data{};
    // for(auto& q: question  ) data += q.dump();
    // for(auto& [sz,v]: ans)
    //     for(auto& a: v) data += a.dump();
    if (create)
        msg = (Message*)malloc(MSG_SIZE + data.size());
    memcpy(msg, buf, MSG_SIZE);
    msg->ID      = ntohs(ID);
    // TODO: msg->Opcode = hton(Opcode);
    msg->QDCOUNT = htons(QDCOUNT);
    msg->ANCOUNT = htons(ANCOUNT);
    msg->NSCOUNT = htons(NSCOUNT);
    msg->ARCOUNT = htons(ARCOUNT);
    return msg;
}

std::ostream& operator<<(std::ostream& os, const Header& m) {
    os  << "ID: " << std::hex << m.ID << '\n'
        << "QR / Opcode: " << std::dec << m.QR << " / " << m.Opcode << '\n'
        << "AA / TC / RD / RA: " << m.AA << m.TC << m.RD << m.RA << '\n'
        << "Z / RCODE: " << m.Z << " / " << m.RCODE << '\n'
        << "QD / AN / NS / AR: " << m.QDCOUNT << " / " << m.ANCOUNT << " / "  << m.NSCOUNT << " / " << m.ARCOUNT << '\n';
    for(auto& q: m.question) os << q << '\n';
    for(auto& [sz,v]: m.ans)
        for(auto& a: v) os << a << '\n';
    return os;
}