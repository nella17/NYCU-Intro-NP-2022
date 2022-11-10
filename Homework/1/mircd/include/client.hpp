#pragma once

#include <string>

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

    Client(int, char*, char*);
    bool regist();
    bool hasRegist();
    bool canRegist();
    bool hasNick();
};

