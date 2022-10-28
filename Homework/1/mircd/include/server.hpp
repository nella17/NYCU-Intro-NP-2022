#pragma once

#include <string>
#include <unordered_map>
#include <sys/epoll.h>

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

    int handle_new_client();
    int handle_client_input(int connfd);
    void disconnect(int connfd);

public:
    int clicnt = 0;
    std::unordered_map<int, Client> cliinfo{};

    Server(int listenport);
    void interactive();
};
