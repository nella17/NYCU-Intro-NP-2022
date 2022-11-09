#include "controller.hpp"

#include "utils.hpp"

void Controller::client_add(int connfd, char* info) {
    printf("* client connected from %s\n", info);
    database.client_info.emplace(connfd, info);
}
bool Controller::client_del(int connfd) {
    auto it = database.client_info.find(connfd);
    if (it == database.client_info.end())
        return false;
    auto client = it->second;
    printf("* client %s disconnected\n", client.info);
    database.client_info.erase(it);
    free(client.info);
    return true;
}

const std::unordered_set<std::string> Controller::unreg_allow{
    "NICK", "USER", "QUIT",
};
const Controller::CmdsMap Controller::cmds{
    // connection
    { "NICK",       { 1, &Controller::nick } },
    { "USER",       { 4, &Controller::user } },
    { "QUIT",       { 0, &Controller::quit } },
    // channel op
    { "JOIN",       { 0, &Controller::join } },
    { "PART",       { 0, &Controller::part } },
    { "TOPIC",      { 0, &Controller::topic } },
    { "NAMES",      { 0, &Controller::names } },
    { "LIST",       { 0, &Controller::list } },
    // send message
    { "PRIVMSG",    { 0, &Controller::privmsg } },
    // misc
    { "PING",       { 0, &Controller::ping } },
    // optional
    { "USERS",      { 0, &Controller::users } },
};

void Controller::call(int connfd, argv_t argv) {
    if (argv.empty())
        return;

    auto cmd = argv.front();
    argv.pop_front();
    auto it = cmds.find(cmd);
    if (it == cmds.end())
        throw ERR_string{ ERR::UNKNOWNCOMMAND, cmd + " :Unknown command" };

    if (!database.isRegist(connfd) and !unreg_allow.count(cmd))
        throw ERR_string{ ERR::NOTREGISTERED, ":You have not registered" };

    auto item = it->second;

    if (argv.size() < item.parm_min)
        throw ERR_string{ ERR::NEEDMOREPARAMS, cmd + " :Not enough parameters" };
    (this->*item.fp)(connfd, argv);
    return;
}

// connection
void Controller::nick(int connfd, argv_t& argv) {
    if (database.isRegist(connfd))
        throw ERR_string{ ERR::ALREADYREGISTRED, ":You may not reregister" };
    auto client = database.client_info.find(connfd)->second;
    client.regist |= HAS_NICK;
    client.nickname = argv[0];
    return;
}
void Controller::user(int connfd, argv_t& argv) {
    if (database.isRegist(connfd))
        throw ERR_string{ ERR::ALREADYREGISTRED, ":You may not reregister" };
    auto client = database.client_info.find(connfd)->second;
    client.regist |= HAS_USER;
    client.username     = argv[0];
    client.hostname     = argv[1];
    client.servername   = argv[2];
    client.realname     = argv[3];
    return;
}
void Controller::quit(int /* connfd */, argv_t& /* argv */) {
    throw EVENT::DISCONNECT;
}

// channel op
void Controller::join(int /* connfd */, argv_t& /* argv */) {
    return;
}
void Controller::part(int /* connfd */, argv_t& /* argv */) {
    return;
}
void Controller::topic(int /* connfd */, argv_t& /* argv */) {
    return;
}
void Controller::names(int /* connfd */, argv_t& /* argv */) {
    return;
}
void Controller::list(int /* connfd */, argv_t& /* argv */) {
    return;
}

// send message
void Controller::privmsg(int /* connfd */, argv_t& /* argv */) {
    return;
}

// misc
void Controller::ping(int /* connfd */, argv_t& /* argv */) {
    return;
}

// optional
void Controller::users(int /* connfd */, argv_t& /* argv */) {
    return;
}
