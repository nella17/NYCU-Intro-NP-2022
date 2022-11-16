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

void Client::changeNick(std::string nick) {
    if (nick.empty() or !(bool)isalpha(nick[0]))
        throw CMD_MSG{ ERR::ERRONEUSNICKNAME, argv_t{ nick, "Erroneous nickname" } };
    if (hasNick())
        sendcmd(connfd, CMD_MSG{ "NICK", argv_t{ nick } }, nickname);
    status |= Client::HAS::NICK;
    nickname = nick;
}

bool Client::in(Channel& channel) {
    return channels.find(channel.name) != channels.end();
}

void Client::join(Channel& channel) {
    channels.emplace(channel.name, channel);
}

void Client::part(Channel& channel) {
    channels.erase(channel.name);
}
