#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>

#include "database.hpp"
#include "utils.hpp"

class Controller {
public:
    void client_add(int, char*);
    bool client_del(int);

    void call(int, argv_t&);

private:
    Database database;

    using func = void (Controller::*)(int, argv_t&);
    struct CmdItem {
        size_t parm_min;
        func fp;
    };
    using CmdsMap = std::unordered_map<std::string, CmdItem>;
    const static std::unordered_set<std::string> unreg_allow;
    const static CmdsMap cmds;

    // connection
    void nick(int, argv_t&);
    void user(int, argv_t&);
    void quit(int, argv_t&);
    // channel op
    void join(int, argv_t&);
    void part(int, argv_t&);
    void topic(int, argv_t&);
    void names(int, argv_t&);
    void list(int, argv_t&);
    // server query & cmd
    // send message
    void privmsg(int, argv_t&);
    // user query
    // misc
    void ping(int, argv_t&);
    // optional
    void users(int, argv_t&);
};
