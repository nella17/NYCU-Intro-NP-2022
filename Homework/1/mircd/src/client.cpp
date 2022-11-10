#include "client.hpp"

Client::Client(int _connfd, char* _info): status(0), connfd(_connfd), info(_info),
    nickname(), username(), hostname(), servername(), realname() {}

bool Client::regist() {
    if ((status & HAS::WELCOME) or !(status & HAS::REGIST))
        return false;
    status |= HAS::WELCOME;
    return true;
}
