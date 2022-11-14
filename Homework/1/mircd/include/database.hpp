#pragma once

#include <string>
#include <unordered_map>
#include "client.hpp"

class Database {
private:

public:
    std::unordered_map<int, Client> clients{};
    std::unordered_map<std::string, Client&> nick_mp{};

    bool nickInUse(std::string);

    Client& getuser(int);
    Client& getuser(std::string);
};
