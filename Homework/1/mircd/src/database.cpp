#include "database.hpp"

bool Database::nickInUse(std::string nick) {
    return nick_fd.find(nick) != nick_fd.end();
}
