#pragma once

#include <map>
#include <vector>

#include "record.hpp"
#include "enum.hpp"
#include "utils.hpp"

class Zone {
    const DN domain;
    std::map<Key, Records> rrs;
public:
    Zone(DN, fs::path);
    bool add(Record);
    Records get(Question);
};
