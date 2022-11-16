#include "controller.hpp"

#include "utils.hpp"

void Controller::client_add(int connfd, char* info, char* host) {
    printf("* client connected from %s\n", info);
    database.clients.try_emplace(connfd, connfd, info, host);
}
bool Controller::client_del(int connfd) {
    auto it = database.clients.find(connfd);
    if (it == database.clients.end())
        return false;
    auto client = it->second;
    printf("* client %s disconnected\n", client.info);
    database.clients.erase(it);
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
    { "JOIN",       { 1, &Controller::join } },
    { "PART",       { 1, &Controller::part } },
    { "TOPIC",      { 1, &Controller::topic } },
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

    auto& client = database.getuser(connfd);
    bool regist = client.hasRegist();
    if (!regist and !unreg_allow.count(cmd))
        throw CMD_MSG{ ERR::NOTREGISTERED, argv_t{ "You have not registered" } };

    auto item = it->second;

    if (argv.size() < item.parm_min)
        throw CMD_MSG{ ERR::NEEDMOREPARAMS, argv_t{ cmd, "Not enough parameters" } };
    (this->*item.fp)(client, argv_t(argv));
    if (!regist and client.regist())
        sendcmds(connfd, WELCOME_CMDS, client.nickname);
    return;
}

// connection
void Controller::nick(Client& client, const argv_t&& argv) {
    if (argv.empty())
        throw CMD_MSG{ ERR::NONICKNAMEGIVEN, argv_t{ "No nickname given" } };
    auto nickname = argv[0];
    if (database.nickInUse(nickname))
        throw CMD_MSG{ ERR::NICKCOLLISION, argv_t{ nickname, "Nickname collision KILL" } };
    if (client.hasNick())
        database.nick_mp.erase(client.nickname);
    database.nick_mp.emplace(nickname, client);
    client.changeNick(nickname);
    return;
}
void Controller::user(Client& client, const argv_t&& argv) {
    if (client.hasRegist())
        throw CMD_MSG{ ERR::ALREADYREGISTRED, argv_t{ "You may not reregister" } };
    client.status |= Client::HAS::USER;
    client.username     = argv[0];
    client.hostname     = argv[1];
    client.servername   = argv[2];
    client.realname     = argv[3];
    return;
}
void Controller::quit(Client& /* client */, const argv_t&& /* argv */) {
    throw EVENT::DISCONNECT;
}

// channel op
void Controller::join(Client& client, const argv_t&& argv) {
    auto name = argv[0];
    auto& channel = database.getchannel(name);
    client.join(channel);
    channel.add(client);
    sendcmd(client.connfd, channel.gettopic(), name);
    names(client, argv_t{ channel.name });
    return;
}
void Controller::part(Client& client, const argv_t&& argv) {
    auto name = argv[0];
    if (!database.hasChannel(name))
        throw CMD_MSG{ ERR::NOSUCHCHANNEL, argv_t{ name, "No such channel" } };
    auto& channel = database.getchannel(name);
    if (!client.in(channel))
        throw CMD_MSG{ ERR::NOTONCHANNEL, argv_t{ name, "You're not on that channel" } };
    client.part(channel);
    auto msg = argv.size() >= 2 ? argv[1] : Channel::default_part_msg;
    channel.del(client, msg);
    return;
}
void Controller::topic(Client& client, const argv_t&& argv) {
    auto name = argv[0];
    if (!database.hasChannel(name))
        throw CMD_MSG{ ERR::NOSUCHCHANNEL, argv_t{ name, "No such channel" } };
    auto& channel = database.getchannel(name);
    if (!client.in(channel))
        throw CMD_MSG{ ERR::NOTONCHANNEL, argv_t{ name, "You're not on that channel" } };
    if (argv.size() < 2) {
        sendcmd(client.connfd, channel.gettopic(), client.nickname);
    } else {
        channel.changeTopic(client.nickname, argv[1]);
    }
    return;
}
void Controller::names(Client&  client, const argv_t&& argv) {
    CMD_MSGS out{};
    if (argv.empty()) {
        for(auto& [name, channel]: database.channels)
            out += channel.getusers();
    } else {
        auto name = argv[0];
        auto& channel = database.getchannel(name);
        out = channel.getusers();
    }
    sendcmds(client.connfd, out, client.nickname);
    return;
}
void Controller::list(Client& /* client */, const argv_t&& /* argv */) {
    return;
}

// send message
void Controller::privmsg(Client& client, const argv_t&& argv) {
    return;
}

// misc
void Controller::ping(Client& client, const argv_t&& argv) {
    if (argv.empty())
        throw CMD_MSG{ ERR::NOORIGIN, argv_t{ "No origin specified" } };
    sendstr(client.connfd, "PONG\n");
    return;
}

// optional
void Controller::users(Client& client, const argv_t&& /* argv */) {
	int mxname = 6; // 123.123.123.123 -> 11 char
	for(auto [fd, cli]: database.clients) {
		int len = cli.nickname.size();
		if (len > mxname) mxname = len;
	}
    int size = mxname + 1 + 8 + 1 + 11 + 1;
    auto buf = new char[size];
    CMD_MSGS out{};
    out.reserve(database.clients.size()+1);
    auto append = [&](int cmd, const char* name, const char* term, const char* host) {
		sprintf(buf, "%-*s %-8s %-21s", mxname, name, term, host);
        out.emplace_back(cmd, argv_t{ buf });
    };
    append(RPL::USERSSTART, "UserID", "Terminal", "Host");
	for(auto [fd, cli]: database.clients) {
        append(RPL::USERS, cli.nickname.c_str(), "-", cli.host);
    }
    free(buf);
    sendcmds(client.connfd, out);
    return;
}
