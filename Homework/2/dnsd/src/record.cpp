#include <string.h>
#include <strings.h>
#include <arpa/inet.h>

#include <iostream>
#include <sstream>

#include "record.hpp"
#include "utils.hpp"

Question::Question(DN _domain, TYPE _type, CLAS _clas):
    domain(_domain), type(_type), clas(_clas) {}

Key Question::key() {
    return Key{ domain, type, clas };
}

std::string Question::dump() {
    auto r = dn2data(domain);
    r += (uint16_t)type;
    r += (uint16_t)clas;
    return r;
}

inline DN parse(TYPE type, std::string data) {
    switch (type) {
        case TYPE::NS:
        case TYPE::CNAME:
            return s2dn(data);
        case TYPE::MX:
            return s2dn(data.substr(data.find(' ')+1));
        default:
            return {};
    }
}

Record::Record(DN _domain, TYPE _type, CLAS _clas, uint32_t _ttl, std::string _data):
    Question(_domain, _type, _clas), ttl(_ttl), data(_data), datadn(parse(_type, _data)) {
    dump();
}

inline std::string dumpSOA(const std::string data) {
    std::stringstream ss(data);
    char MNAME[256], RNAME[256];
    uint32_t SERIAL, REFRESH, RETRY, EXPIRE, MINIMUM;
    ss >> MNAME >> RNAME >> SERIAL >> REFRESH >> RETRY >> EXPIRE >> MINIMUM;
    std::string rdata = "";
    rdata += dn2data(s2dn(MNAME));
    rdata += dn2data(s2dn(RNAME));
    rdata += SERIAL;
    rdata += REFRESH;
    rdata += RETRY;
    rdata += EXPIRE;
    rdata += MINIMUM;
    return rdata;
}

inline std::string dumpMX(const std::string data) {
    std::stringstream ss(data);
    uint16_t PREFERENCE;
    char EXCHANGE[256];
    ss >> PREFERENCE >> EXCHANGE;
    std::string rdata = "";
    rdata += PREFERENCE;
    rdata += dn2data(s2dn(EXCHANGE));
    return rdata;
}


std::string Record::rdata() {
    switch (type) {
        case TYPE::A:
            return inet_pton(AF_INET, data);
        case TYPE::AAAA:
            return inet_pton(AF_INET6, data);
        case TYPE::NS:
        case TYPE::CNAME:
            return dn2data(s2dn(data));
        case TYPE::SOA:
            return dumpSOA(data);
        case TYPE::MX:
            return dumpMX(data);
        case TYPE::TXT:
            return (char)(uint8_t)data.size() + data;
    }
    return "";
}

std::string Record::dump() {
    auto r = dn2data(domain);
    r += (uint16_t)type;
    r += (uint16_t)clas;
    r += ttl;
    auto rd = rdata();
    r += (uint16_t)rd.size();
    r += rd;
    return r;
}

std::ostream& operator<<(std::ostream& os, const Question& q) {
    os << q.domain
        << ' ' << enum_name(q.type)
        << ' ' << enum_name(q.clas);
    return os;
}
std::ostream& operator<<(std::ostream& os, const Record& rr) {
    os << rr.domain
        << ' ' << enum_name(rr.type)
        << ' ' << enum_name(rr.clas)
        << ' ' << std::dec << rr.ttl
        << ' ' << rr.data
        << ' ' << rr.datadn;
    return os;
}

Records niplike(DN domain, DN base) {
    DN sub = domain - base;
    reverse(sub.begin(), sub.end());
    bool can = sub.size() == 5 and sub[4].size() < 62;
    if (can) {
        sub.pop_back();
        for(auto label: sub) {
            can &= label.size() <= 3;
            for(auto x: label)
                can &= '0' <= x and x <= '9';
        }
    }
    if (!can) return {};
    reverse(sub.begin(), sub.end());
    auto data = dn2s(sub);
    data.pop_back();
    if (VERBOSE >= 1)
        std::cout << "NIP: " << domain << " -> " << data << std::endl;
    Record rr(domain, TYPE::A, CLAS::IN, 1, data);
    return { rr };
}
