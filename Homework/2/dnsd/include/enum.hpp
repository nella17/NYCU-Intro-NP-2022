#pragma once

#include "magic_enum.hpp"
using magic_enum::enum_name;
using magic_enum::enum_cast;

enum class TYPE: uint16_t {
    A     =  1,
    AAAA  = 28,
    NS    =  2,
    CNAME =  5,
    SOA   =  6,
    MX    = 15,
    TXT   = 16,
};

enum class CLAS: uint16_t {
    IN = 1,
    CS = 2,
    CH = 3,
    HS = 4,
};

template<typename T>
inline TYPE s2type(T s) { return enum_cast<TYPE>(std::string(s)).value(); }

template<typename T>
inline CLAS s2clas(T s) { return enum_cast<CLAS>(std::string(s)).value(); }
