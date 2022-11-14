#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>

#include "database.hpp"
#include "utils.hpp"

class Controller {
public:
    void client_add(int, char*, char*);
    bool client_del(int);

    void call(int, argv_t&);

private:
    Database database;

    using func = void (Controller::*)(Client&, const argv_t&&);
    struct CmdItem {
        size_t parm_min;
        func fp;
    };
    using CmdsMap = std::unordered_map<std::string, CmdItem>;
    const static std::unordered_set<std::string> unreg_allow;
    const static CmdsMap cmds;

    // connection
    void nick(Client&, const argv_t&&);
    void user(Client&, const argv_t&&);
    void quit(Client&, const argv_t&&);
    // channel op
    void join(Client&, const argv_t&&);
    void part(Client&, const argv_t&&);
    void topic(Client&, const argv_t&&);
    void names(Client&, const argv_t&&);
    void list(Client&, const argv_t&&);
    // server query & cmd
    // send message
    void privmsg(Client&, const argv_t&&);
    // user query
    // misc
    void ping(Client&, const argv_t&&);
    // optional
    void users(Client&, const argv_t&&);
};
