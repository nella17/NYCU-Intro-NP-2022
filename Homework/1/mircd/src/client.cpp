#include "client.hpp"

Client::Client(int _connfd, char* _info): status(0), connfd(_connfd), info(_info),
    nickname(), username(), hostname(), servername(), realname() {}

bool Client::regist() {
    if (isRegist() or !canRegist())
        return false;
    status |= HAS::WELCOME;
    return true;
}

bool Client::isRegist() {
    return (status & HAS::WELCOME) == HAS::WELCOME;
}
bool Client::canRegist() {
    return (status & HAS::REGIST) == HAS::REGIST;
}
