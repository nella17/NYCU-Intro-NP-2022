#pragma once

#include <string>
#include <vector>

#include "enum.hpp"

using DN = std::vector<std::string>;

class Record {
public:
    const DN domain;
    const TYPE type;
    const CLAS clas;
    const uint32_t ttl;
    const std::string data, rdata;
    Record(DN, TYPE, CLAS, uint32_t, std::string);
};

std::string rd2data(const TYPE, const std::string);
std::ostream& operator<<(std::ostream&, const Record&);

DN s2dn(std::string);
std::string dn2data(DN);
