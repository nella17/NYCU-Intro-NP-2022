#pragma once

#include <unordered_map>
#include "client.hpp"

class Database {
private:

public:
    std::unordered_map<int, Client> client_info{};
    std::unordered_map<std::string, int> nick_fd{};

    bool nickInUse(std::string);
};
