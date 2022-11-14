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
