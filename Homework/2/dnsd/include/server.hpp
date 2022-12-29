#pragma once

#include <stdint.h>

#include "config.hpp"
#include "header.hpp"

class Server {
public:
    Server(uint16_t, const char[]);
    void interactive();
    std::string query(std::string);
    std::string query(Header&);
    std::string forward(std::string);

private:
    Config config;
    int listenfd, connfd;
};
