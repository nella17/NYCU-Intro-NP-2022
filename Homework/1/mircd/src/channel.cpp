#include "channel.hpp"

Channel::Channel(std::string _name):
    name(_name), topic(""), users() {
        if (name.empty() or name.size() > 50 or
                (name[0] != '#' and name[0] != '&' and name[0] != '+'))
            throw CMD_MSG{ ERR::NOSUCHCHANNEL, argv_t{ name, "No such channel" } };
    }

void Channel::add(Client& client) {
    users.emplace(client.connfd, client);
}

void Channel::del(int connfd) {
    users.erase(connfd);
}
void Channel::del(Client& client) {
    users.erase(client.connfd);
}

CMD_MSG Channel::gettopic() {
    if (topic.empty())
        return CMD_MSG{ RPL::NOTOPIC, argv_t{ name, "No topic is set" } };
    else
        return CMD_MSG{ RPL::TOPIC, argv_t{ name, topic } };
}

std::string Channel::usersstr() {
    std::string str;
    for(const auto& [fd,user]: users) {
        if (!str.empty()) str.append(" ");
        str.append(user.nickname);
    }
    return str;
}

CMD_MSGS Channel::getusers() {
    CMD_MSGS out{};
    if (!users.empty())
        out.emplace_back(RPL::NAMREPLY, argv_t{ name, usersstr() });
    out.emplace_back(RPL::ENDOFNAMES, argv_t{ name, "End of /NAMES list" } );
    return out;
}
