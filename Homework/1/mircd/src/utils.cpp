#include "utils.hpp"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>

std::vector<CMD_MSG> WELCOME_CMDS{
    CMD_MSG{ RPL::WELCOME, argv_t{ "Welcome to the minimized IRC daemon!" } },
    CMD_MSG{ RPL::LUSERCLIENT, argv_t{ "There are ? users and ? invisible on ? servers" } },
    CMD_MSG{ RPL::MOTDSTART, argv_t{ R"(-  Hello, World!                       )" } },
    CMD_MSG{ RPL::MOTDSTART, argv_t{ R"(-               @                    _ )" } },
    CMD_MSG{ RPL::MOTDSTART, argv_t{ R"(-   ____  ___   _   _ _   ____.     | |)" } },
    CMD_MSG{ RPL::MOTDSTART, argv_t{ R"(-  /  _ `'_  \ | | | '_/ /  __|  ___| |)" } },
    CMD_MSG{ RPL::MOTDSTART, argv_t{ R"(-  | | | | | | | | | |   | |    /  _  |)" } },
    CMD_MSG{ RPL::MOTDSTART, argv_t{ R"(-  | | | | | | | | | |   | |__  | |_| |)" } },
    CMD_MSG{ RPL::MOTDSTART, argv_t{ R"(-  |_| |_| |_| |_| |_|   \____| \___,_|)" } },
    CMD_MSG{ RPL::MOTDSTART, argv_t{ R"(-  minimized internet relay chat daemon)" } },
    CMD_MSG{ RPL::MOTDSTART, argv_t{ R"(-                                      )" } },
    CMD_MSG{ RPL::ENDOFMOTD, argv_t{ "End of message of the day" } },
};

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
        char tmp[4];
        sprintf(tmp, "%03d", cmd);
        buf.append(tmp);
        for(auto it = msg.begin(); it != msg.end(); it++) {
            buf.append(" ");
            if (next(it) == msg.end())
                buf.append(":");
            buf.append(*it);
        }
        buf.append("\n");
    }
    sendstr(fd, buf);
}

char* sock_info(const struct sockaddr_in* sock) {
    char buf[64], host[16];
    sprintf(buf, "%s:%d", inet_ntop(AF_INET, &sock->sin_addr, host, 16), ntohs(sock->sin_port));
    return strdup(buf);
}
char* sock_host(const struct sockaddr_in* sock) {
    char host[16];
    inet_ntop(AF_INET, &sock->sin_addr, host, 16);
    return strdup(host);
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
