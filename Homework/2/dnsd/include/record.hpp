#pragma once

#include <string>
#include <vector>

#include "enum.hpp"
#include "dn.hpp"

using Key = std::tuple<DN, TYPE, CLAS>;

class Question {
public:
    const DN domain;
    const TYPE type;
    const CLAS clas;
    Question(DN, TYPE, CLAS);
    Key key();
    virtual std::string dump();
};

class Record: public Question {
public:
    const uint32_t ttl;
    const std::string data, rdata;
    const DN datadn;
    Record(DN, TYPE, CLAS, uint32_t, std::string);
    virtual std::string dump();
};

using Records = std::vector<Record>;

std::ostream& operator<<(std::ostream&, const Question&);
std::ostream& operator<<(std::ostream&, const Record&);

Records niplike(DN, DN);
std::string Rdata(TYPE, std::string);
