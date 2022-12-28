#include <iostream>
#include <string_view>
#include <ranges>

#include "dn.hpp"

std::ostream& operator<<(std::ostream& os, const DN& domain) {
    for(auto label: domain | std::views::reverse)
        os << label << '.';
    return os;
}

bool operator%(const DN& a, const DN& b) {
    if (a.size() < b.size())
        return false;
    size_t sz = b.size();
    for(size_t i = 0; i < sz; i++)
        if (a[i] != b[i])
            return false;
    return true;
}

DN s2dn(std::string name) {
    if (name == "@")
        return {};
    auto v = name
        | std::ranges::views::split('.')
        | std::ranges::views::transform([](auto &&rng) {
            return std::string_view(&*rng.begin(), std::ranges::distance(rng));
        });
    DN domain(v.begin(), v.end());
    std::reverse(domain.begin(), domain.end());
    return domain;
}

std::string dn2data(DN domain) {
    std::string data;
    for(auto label: domain | std::views::reverse) {
        data += (char)(uint8_t)label.size();
        data += label;
    }
    data += '\0';
    return data;
}

DN data2dn(char*& data) {
    DN domain{};
    while (*data) {
        auto sz = (size_t)(uint8_t)*data;
        domain.emplace_back(data + 1, sz);
        data += 1 + sz;
    }
    data += 1;
    std::reverse(domain.begin(), domain.end());
    return domain;
}
