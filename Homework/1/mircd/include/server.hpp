#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unordered_map>
#include <string>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include "utils.hpp"

#define MAX_EVENTS 1024
#define MAX_LINE 1024

struct Client {
    char *info;
    std::string name;
    Client(char* _info = nullptr): info(_info), name("") {}
};

class Server {
private:
    int listenfd, epollfd;
    struct epoll_event events[MAX_EVENTS];

    int handle_new_client();
    int handle_client_input(int connfd);

public:
    int clicnt = 0;
    std::unordered_map<int, Client> cliinfo{};

    Server(int listenport);
    void interactive();
};
