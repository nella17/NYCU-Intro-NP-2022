#pragma once

#include <unordered_map>
#include "client.hpp"

class Database {
private:

public:
    std::unordered_map<int, Client> client_info{};

    bool isRegist(int);
};
