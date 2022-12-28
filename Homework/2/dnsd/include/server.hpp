#pragma once

#include <stdint.h>

#include "config.hpp"

class Server {
public:
    Server(uint16_t, const char[]);
    void interactive();
    std::string query(std::string);
    std::string forward(std::string);

private:
    Config config;
    int listenfd, connfd;
};
