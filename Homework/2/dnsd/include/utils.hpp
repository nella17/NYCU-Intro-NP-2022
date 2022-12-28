#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>

#include <string>
#include <sstream>
#include <string_view>
#include <ranges>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

#include "enum.hpp"

constexpr int VERBOSE = 2;

void fail(const char* s);
void bt();

int connect(const char* host, uint16_t port);

char* sock_info(const struct sockaddr_in* sock);
char* sock_host(const struct sockaddr_in* sock);

std::string inet_pton(int, std::string);

template<typename T>
std::vector<T> operator+(const std::vector<T>& a, const std::vector<T>& b) {
    std::vector<T> r{};
    r.reserve(a.size() + b.size());
    std::copy(a.begin(), a.end(), std::back_inserter(r));
    std::copy(b.begin(), b.end(), std::back_inserter(r));
    return r;
}

inline std::string& operator+=(std::string& s, uint16_t t) {
    t = htons(t);
    s.append((char*)&t, sizeof(t));
    return s;
}
inline std::string& operator+=(std::string& s, uint32_t t) {
    t = htonl(t);
    s.append((char*)&t, sizeof(t));
    return s;
}

std::string hexdump(std::string);

template<typename T>
struct PAD {
    int pad;
    T data;
};

template<typename T>
inline std::ostream& operator<<(std::ostream& os, PAD<T> o) {
    std::stringstream ss;
    ss << o.data;
    for(auto s: ss.view()
            | std::ranges::views::split('\n')
            | std::ranges::views::transform([](auto &&rng) {
                return std::string_view(&*rng.begin(), std::ranges::distance(rng));
            }) )
        os << std::string(o.pad, ' ') << s << '\n';
    return os;
}
