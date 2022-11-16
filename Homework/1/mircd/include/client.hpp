#pragma once

#include <string>
#include <unordered_map>

class Client;

#include "channel.hpp"

class Client {
public:
    struct HAS {
        constexpr static char NICK     = 1;
        constexpr static char USER     = 2;
        constexpr static char WELCOME  = 4;
        constexpr static char REGIST   = NICK | USER;
    };

    char status;
    int connfd;
    char* const info;
    char* const host;
    std::string nickname, username, hostname, servername, realname;
    std::unordered_map<std::string, Channel&> channels;

    Client(int, char*, char*);
    bool regist();
    bool hasRegist();
    bool canRegist();
    bool hasNick();
    void changeNick(std::string);

    bool in(Channel&);
    void join(Channel&);
    void part(Channel&);
};

