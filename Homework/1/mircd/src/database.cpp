#include "database.hpp"

bool Database::nickInUse(std::string nick) {
    return nick_mp.find(nick) != nick_mp.end();
}

Client& Database::get(int fd) {
    return client_info.find(fd)->second;
}
Client& Database::get(std::string nick) {
    return nick_mp.find(nick)->second;
}
