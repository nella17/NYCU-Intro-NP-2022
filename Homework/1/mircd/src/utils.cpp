#include "utils.hpp"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>

void fail(const char* s) {
    if (errno) perror(s);
    else fprintf(stderr, "%s: unknown error", s);
#ifdef DEBUG
    exit(-1);
#endif
}

void sendstr(int fd, std::string buf) {
#ifdef DEBUG
    std::cerr << buf << std::flush;
#endif
    send(fd, buf.c_str(), buf.size(), SEND_FLAG);
}

void sendcmd(int fd, CMD_MSG cmd) {
    sendcmds(fd, { cmd });
}
void sendcmds(int fd, std::vector<CMD_MSG> cmds) {
    std::string buf = "";
    for(auto [cmd, msg]: cmds) {
        buf.append(std::to_string(cmd));
        for(auto it = msg.begin(); it != msg.end(); it++) {
            buf.append(" ");
            if (next(it) == msg.end())
                buf.append(":");
            buf.append(*it);
        }
    }
    sendstr(fd, buf);
}

char* sock_info(const struct sockaddr_in* sock) {
    char buf[64], host[16];
    sprintf(buf, "%s:%d", inet_ntop(AF_INET, &sock->sin_addr, host, 16), ntohs(sock->sin_port));
    return strdup(buf);
}

argv_t parse(const char* _buf, int size) {
    argv_t cmds{};
    char* buf = strdup(_buf);
    while (size and (buf[size-1] == '\r' or buf[size-1] == '\n'))
        size--;
    buf[size] = '\0';
    for(int i = 0, j = 0; i <= size; i++) {
        if (buf[i] == ' ' or buf[i] == ':' or buf[i] == '\0') {
            bool iscolon = buf[i] == ':';
            buf[i] = '\0';
            if (buf[j])
                cmds.emplace_back(buf+j);
            j = i+1;
            if (iscolon) {
                cmds.emplace_back(buf+j);
                break;
            }
        }
    }
    free(buf);
    return cmds;
}
