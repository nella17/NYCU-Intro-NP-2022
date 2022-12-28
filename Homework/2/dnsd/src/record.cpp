#include <string.h>
#include <strings.h>
#include <arpa/inet.h>

#include <iostream>
#include <string_view>
#include <sstream>
#include <ranges>

#include "record.hpp"
#include "utils.hpp"

Record::Record(DN _domain, TYPE _type, CLAS _clas, uint32_t _ttl, std::string _data):
    domain(_domain), type(_type), clas(_clas), ttl(_ttl), data(_data) {}

inline std::string parseSOA(const std::string data) {
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

inline std::string parseMX(const std::string data) {
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
    std::string ret;
    switch (type) {
        case TYPE::A:
            ret = inet_pton(AF_INET, data);
            break;
        case TYPE::AAAA:
            ret = inet_pton(AF_INET6, data);
            break;
        case TYPE::NS:
        case TYPE::CNAME:
            ret = dn2data(s2dn(data));
            break;
        case TYPE::SOA:
            ret = parseSOA(data);
            break;
        case TYPE::MX:
            ret = parseMX(data);
            break;
        case TYPE::TXT:
            ret = data;
    }
    return ret;
}

std::ostream& operator<<(std::ostream& os, const DN& domain) {
    for(auto label: domain | std::views::reverse)
        os << label << '.';
    return os;
}
std::ostream& operator<<(std::ostream& os, const Record& rr) {
    os << rr.domain
        << ' ' << enum_name(rr.type)
        << ' ' << enum_name(rr.clas)
        << ' ' << std::dec << rr.ttl
        << ' ' << rr.data;
    return os;
}

DN s2dn(std::string name) {
    if (name == "@")
        return {};
    auto v = name
        | std::ranges::views::split('.')
        | std::ranges::views::transform([](auto &&rng) {
            return std::string_view(&*rng.begin(), std::ranges::distance(rng));
        });
    DN domain(v.begin(), v.end());
    std::reverse(domain.begin(), domain.end());
    return domain;
}

std::string dn2data(DN domain) {
    std::string data;
    for(auto label: domain | std::views::reverse) {
        data += (char)(uint8_t)label.size();
        data += label;
    }
    data += '\0';
    return data;
}
