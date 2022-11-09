#pragma once

#include <unordered_map>
#include <sys/epoll.h>

#include "controller.hpp"

constexpr int  MAX_EVENTS = 1024;
constexpr int  MAX_LINE  = 1024;

class Server {
public:
    Server(int listenport);
    void interactive();

private:
    int listenfd, epollfd;
    int clicnt = 0;
    Controller controller;

    int handle_new_client();
    int handle_client_input(int connfd);
    void disconnect(int connfd);
};
