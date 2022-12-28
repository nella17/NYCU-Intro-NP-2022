#pragma once

#include <string>
#include <vector>

using DN = std::vector<std::string>;

std::ostream& operator<<(std::ostream&, const DN&);
bool operator%(const DN&, const DN&);

DN s2dn(std::string);
std::string dn2data(DN);

DN data2dn(char*&);
