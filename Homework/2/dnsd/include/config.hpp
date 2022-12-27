#pragma once

#include <stdint.h>

#include "record.hpp"

class Config {
public:
    uint32_t forwardIP;

    Config(const char[]);
};
