#pragma once

#include <string>
#include <unordered_map>
#include "client.hpp"

class Database {
private:

public:
    std::unordered_map<int, Client> client_info{};
    std::unordered_map<std::string, Client&> nick_mp{};

    bool nickInUse(std::string);

    Client& get(int);
    Client& get(std::string);
};
