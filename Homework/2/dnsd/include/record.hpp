#pragma once

#include <string>
#include <vector>

#include "enum.hpp"

using DN = std::vector<std::string>;

class Question {
public:
    const DN domain;
    const TYPE type;
    const CLAS clas;
    Question(DN, TYPE, CLAS);
};

class Record {
public:
    const DN domain;
    const TYPE type;
    const CLAS clas;
    const uint32_t ttl;
    const std::string data;
    Record(DN, TYPE, CLAS, uint32_t, std::string);
    std::string rdata();
};

std::ostream& operator<<(std::ostream&, const DN&);
std::ostream& operator<<(std::ostream&, const Question&);
std::ostream& operator<<(std::ostream&, const Record&);

DN s2dn(std::string);
std::string dn2data(DN);
DN data2dn(char*&);
