#include "database.hpp"

bool Database::isRegist(int connfd) {
    auto it = client_info.find(connfd);
    if (it == client_info.end())
        return false;
    return it->second.status == Client::HAS::REGIST;
}
