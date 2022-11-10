#pragma once

#include <sys/socket.h>
#include <string>
#include <deque>
#include <vector>


#include "enums.hpp"
using argv_t = std::deque<std::string>;
using CMD_MSG = std::pair<ERR, argv_t>;

enum EVENT {
    DISCONNECT,
};

constexpr int SEND_FLAG = MSG_NOSIGNAL | MSG_DONTWAIT;

void fail(const char* s);

void sendstr(int fd, const char* buf);
void sendstr(int fd, std::string buf);

void sendcmd(int fd, CMD_MSG cmd);
void sendcmds(int fd, std::vector<CMD_MSG> cmds);

char* sock_info(const struct sockaddr_in* sock);

argv_t parse(const char* buf, int size);
