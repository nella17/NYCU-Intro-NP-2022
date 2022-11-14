#include "controller.hpp"

#include "utils.hpp"

void Controller::client_add(int connfd, char* info, char* host) {
    printf("* client connected from %s\n", info);
    database.client_info.try_emplace(connfd, connfd, info, host);
}
bool Controller::client_del(int connfd) {
    auto it = database.client_info.find(connfd);
    if (it == database.client_info.end())
        return false;
    auto client = it->second;
    printf("* client %s disconnected\n", client.info);
    database.client_info.erase(it);
    database.nick_mp.erase(client.nickname);
    free(client.info);
    free(client.host);
    return true;
}

const std::unordered_set<std::string> Controller::unreg_allow{
    "NICK", "USER", "QUIT",
};
const Controller::CmdsMap Controller::cmds{
    // connection
    { "NICK",       { 0, &Controller::nick } },
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

void Controller::call(int connfd, argv_t& argv) {
    if (argv.empty())
        return;

    auto cmd = argv.front();
    argv.pop_front();
    auto it = cmds.find(cmd);
    if (it == cmds.end())
        throw CMD_MSG{ ERR::UNKNOWNCOMMAND, argv_t{ cmd, "Unknown command" } };

    auto& client = database.get(connfd);
    bool regist = client.hasRegist();
    if (!regist and !unreg_allow.count(cmd))
        throw CMD_MSG{ ERR::NOTREGISTERED, argv_t{ "You have not registered" } };

    auto item = it->second;

    if (argv.size() < item.parm_min)
        throw CMD_MSG{ ERR::NEEDMOREPARAMS, argv_t{ cmd, "Not enough parameters" } };
    (this->*item.fp)(client, argv);
    if (!regist and client.regist())
        sendcmds(connfd, WELCOME_CMDS, client.nickname);
    return;
}

// connection
void Controller::nick(Client& client, argv_t& argv) {
    if (argv.empty())
        throw CMD_MSG{ ERR::NONICKNAMEGIVEN, argv_t{ "No nickname given" } };
    auto nickname = argv[0];
    if (database.nickInUse(nickname))
        throw CMD_MSG{ ERR::NICKCOLLISION, argv_t{ nickname, "Nickname collision KILL" } };
    if (client.hasNick())
        database.nick_mp.erase(client.nickname);
    database.nick_mp.emplace(nickname, client);
    client.status |= Client::HAS::NICK;
    client.nickname = nickname;
    return;
}
void Controller::user(Client& client, argv_t& argv) {
    if (client.hasRegist())
        throw CMD_MSG{ ERR::ALREADYREGISTRED, argv_t{ "You may not reregister" } };
    client.status |= Client::HAS::USER;
    client.username     = argv[0];
    client.hostname     = argv[1];
    client.servername   = argv[2];
    client.realname     = argv[3];
    return;
}
void Controller::quit(Client& /* client */, argv_t& /* argv */) {
    throw EVENT::DISCONNECT;
}

// channel op
void Controller::join(Client& /* client */, argv_t& /* argv */) {
    return;
}
void Controller::part(Client& /* client */, argv_t& /* argv */) {
    return;
}
void Controller::topic(Client& /* client */, argv_t& /* argv */) {
    return;
}
void Controller::names(Client& /* client */, argv_t& /* argv */) {
    return;
}
void Controller::list(Client& /* client */, argv_t& /* argv */) {
    return;
}

// send message
void Controller::privmsg(Client& client, argv_t& argv) {
    return;
}

// misc
void Controller::ping(Client& client, argv_t& argv) {
    if (argv.empty())
        throw CMD_MSG{ ERR::NOORIGIN, argv_t{ "No origin specified" } };
    sendstr(client.connfd, "PONG\n");
    return;
}

// optional
void Controller::users(Client& client, argv_t& /* argv */) {
	int mxname = 6; // 123.123.123.123 -> 11 char
	for(auto [fd, cli]: database.client_info) {
		int len = cli.nickname.size();
		if (len > mxname) mxname = len;
	}
    int size = mxname + 1 + 8 + 1 + 11 + 1;
    auto buf = new char[size];
    std::vector<CMD_MSG> out{};
    auto append = [&](int cmd, const char* name, const char* term, const char* host) {
		sprintf(buf, "%-*s %-8s %-21s", mxname, name, term, host);
        out.emplace_back(cmd, argv_t{ buf });
    };
    append(RPL::USERSSTART, "UserID", "Terminal", "Host");
	for(auto [fd, cli]: database.client_info) {
        append(RPL::USERS, cli.nickname.c_str(), "-", cli.host);
    }
    free(buf);
    sendcmds(client.connfd, out, client.nickname);
    return;
}
