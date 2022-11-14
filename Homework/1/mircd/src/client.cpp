#include "client.hpp"

Client::Client(int _connfd, char* _info, char* _host):
    status(0), connfd(_connfd), info(_info), host(_host),
    nickname(), username(), hostname(), servername(), realname(),
    channels() {}

bool Client::regist() {
    if (hasRegist() or !canRegist())
        return false;
    status |= HAS::WELCOME;
    return true;
}

bool Client::hasRegist() {
    return (status & HAS::WELCOME) == HAS::WELCOME;
}
bool Client::canRegist() {
    return (status & HAS::REGIST) == HAS::REGIST;
}

bool Client::hasNick() {
    return (status & HAS::NICK) == HAS::NICK;
}
