#pragma once

#include <stdint.h>

#include "record.hpp"

class Config {
public:
    char forwardIP[20];

    Config(const char[]);
};
