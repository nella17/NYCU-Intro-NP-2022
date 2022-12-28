#pragma once

#include <stdint.h>
#include <map>

#include "zone.hpp"

class Config {
public:
    char forwardIP[20];
    std::map<DN, Zone> zones;

    Config(const char[]);
    bool has(DN) const;
    const Zone& get(DN) const;
};
