#pragma once

#include <stdint.h>

class Server {
public:
    Server(uint16_t, const char[]);
    void interactive();

private:
    int listenfd;
};
