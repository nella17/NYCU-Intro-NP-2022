#pragma once

#include <unordered_map>
#include <string>

class Channel;

#include "client.hpp"
#include "utils.hpp"

class Channel {
private:
public:
    static const std::string default_part_msg;

    std::string name, topic;
    std::unordered_map<int, Client&> users;

    Channel(std::string);
    void add(Client&);
    void del(int, std::string = default_part_msg);
    void del(Client&, std::string = default_part_msg);
    void changeTopic(std::string, std::string);

    CMD_MSG gettopic();
    std::string usersstr();
    CMD_MSGS getusers();
};
