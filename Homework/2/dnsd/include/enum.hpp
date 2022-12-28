#pragma once

#include <iostream>
#include <concepts>

#include "magic_enum.hpp"
using magic_enum::enum_name;
using magic_enum::enum_cast;
using magic_enum::enum_contains;

#include "utils.hpp"

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

template<typename E, typename T, std::enable_if_t<!std::is_integral<T>::value, int> = 0>
inline E T2E(T) { return E(-1); }
template<typename E, typename T, std::enable_if_t<std::is_integral<T>::value, int> = 0>
inline E T2E(T v) { return E(v); }

template<typename E, typename T>
inline E v2e(T v) {
    auto e = enum_cast<E>(v);
    if (e.has_value())
        return e.value();
    constexpr auto entries = magic_enum::enum_entries<E>();
    for(auto& [_e,_s]: entries)
        std::cerr << ' ' << static_cast<magic_enum::underlying_type_t<E>>(_e) << ' ' << _s << '\n';
    std::cerr << " Query: " << v << std::endl;
    bt();
    return T2E<E>(v);
}

template<typename T>
inline TYPE v2type(T s) { return v2e<TYPE>(s); }

template<typename T>
inline CLAS v2clas(T s) { return v2e<CLAS>(s); }
