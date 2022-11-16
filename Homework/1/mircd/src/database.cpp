#include "database.hpp"

bool Database::nickInUse(std::string nick) {
    return nick_mp.find(nick) != nick_mp.end();
}

Client& Database::getuser(int fd) {
    return clients.find(fd)->second;
}
Client& Database::getuser(std::string nick) {
    return nick_mp.find(nick)->second;
}

Channel& Database::getchannel(std::string name) {
    auto it = channels.find(name);
    if (it == channels.end())
        it = channels.emplace(name, name).first;
    return it->second;
}
