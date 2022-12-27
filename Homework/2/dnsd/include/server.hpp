#pragma once

#include <stdint.h>

#include "config.hpp"

class Server {
public:
    Server(uint16_t, const char[]);
    void interactive();

private:
    Config config;
    int listenfd;
};
