#pragma once

#include <unordered_map>
#include <string>

class Channel;

#include "client.hpp"
#include "utils.hpp"

class Channel {
private:
public:
    std::string name, topic;
    std::unordered_map<int, Client&> users;

    Channel(std::string);
    void add(Client&);
    void del(int);
    void del(Client&);

    CMD_MSG gettopic();
    std::string usersstr();
    CMD_MSGS getusers();
};
