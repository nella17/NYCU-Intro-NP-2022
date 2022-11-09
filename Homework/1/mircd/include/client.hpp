#pragma once

#include <string>

constexpr char HAS_NICK = 1;
constexpr char HAS_USER = 2;
constexpr char HAS_REGIST = HAS_NICK | HAS_USER;

class Client {
public:
    char regist;
    char* const info;
    std::string nickname, username, hostname, servername, realname;
    Client(char* _info);
};

