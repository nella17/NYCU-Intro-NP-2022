#pragma once

#include <sys/socket.h>
#include <string>
#include <deque>
#include <vector>
#include <variant>

#include "enums.hpp"
using argv_t = std::deque<std::string>;
using CMD = std::variant<std::string, int>;
using CMD_MSG = std::pair<CMD, argv_t>;
using CMD_MSGS = std::vector<CMD_MSG>;

extern CMD_MSGS WELCOME_CMDS;

enum EVENT {
    DISCONNECT,
};

constexpr int SEND_FLAG = MSG_NOSIGNAL | MSG_DONTWAIT;

void fail(const char* s);

void sendstr(int fd, std::string buf);

void sendcmd(int fd, CMD_MSG cmd, std::string nick = "");
void sendcmds(int fd, CMD_MSGS cmds, std::string nick = "");

char* sock_info(const struct sockaddr_in* sock);
char* sock_host(const struct sockaddr_in* sock);

argv_t parse(const char* buf, int size);
