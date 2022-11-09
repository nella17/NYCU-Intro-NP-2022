#pragma once

#include <sys/socket.h>
#include <string>
#include <deque>

#include "enums.hpp"
using ERR_string = std::pair<ERR, std::string>;

enum EVENT {
    DISCONNECT,
};

constexpr int SEND_FLAG = MSG_NOSIGNAL | MSG_DONTWAIT;

void fail(const char* s);

void sendstr(int fd, const char* buf);
void sendstr(int fd, std::string buf);

char* sock_info(const struct sockaddr_in* sock);

using argv_t = std::deque<std::string>;
argv_t parse(const char* buf, int size);
