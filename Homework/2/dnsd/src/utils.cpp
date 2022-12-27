#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <ranges>

#include "utils.hpp"

void fail(const char* s) {
    if (errno) perror(s);
    else fprintf(stderr, "%s: unknown error", s);
#ifdef DEBUG
    exit(-1);
#endif
}

int connect(const char* host, uint16_t port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0)
        perror("sockfd"), exit(-1);

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0)
        fail("inet_pton");

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        fail("connect");

    return sockfd;
}

char* sock_info(const struct sockaddr_in* sock) {
    char buf[64], host[16];
    sprintf(buf, "%s:%d", inet_ntop(AF_INET, &sock->sin_addr, host, 16), ntohs(sock->sin_port));
    return strdup(buf);
}
char* sock_host(const struct sockaddr_in* sock) {
    char host[16];
    inet_ntop(AF_INET, &sock->sin_addr, host, 16);
    return strdup(host);
}

std::string inet_pton(int af, std::string data) {
    size_t sz = 0;
    switch (af) {
    case AF_INET:
        sz = sizeof(struct in_addr);
        break;
    case AF_INET6:
        sz = sizeof(struct in6_addr);
        break;
    }
    if (!sz) return nullptr;
    char buf[sz];
    bzero(buf, sz);
    assert(inet_pton(af, data.c_str(), buf) == 1);
    return { buf, sz };
}

std::string hexdump(std::string data) {
    std::stringstream ss;
    size_t sz = data.size();
    ss << std::hex << std::setfill('0');
    for(size_t i = 0, j; i < sz; i += 16) {
        if (i) ss << '\n';
        ss << std::setw(4) << i << ':';
        for(j = 0; j < 16 and i + j < sz; j++)
            ss << ' ' << std::setw(2) << (uint32_t)(uint8_t)data[i+j];
        for(; j < 16; j++) ss << "   ";
        ss << ' ';
        for(j = 0; j < 16 and i + j < sz; j++)
            ss << (isprint(data[i+j]) ? data[i+j] : '.');
        for(; j < 16; j++) ss << " ";
    }
    ss << "  " << std::dec << sz << '\n';
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, HEX hex) {
    auto dump = hexdump(hex.data);
    for(auto s: dump
            | std::ranges::views::split('\n')
            | std::ranges::views::transform([](auto &&rng) {
                return std::string_view(&*rng.begin(), std::ranges::distance(rng));
            }) )
        os << std::string(hex.pad, ' ') << s << '\n';
    return os;
}
