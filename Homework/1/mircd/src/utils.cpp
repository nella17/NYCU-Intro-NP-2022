#include "utils.hpp"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void fail(const char* s) {
    if (errno) perror(s);
    else fprintf(stderr, "%s: unknown error", s);
#ifdef DEBUG
    exit(-1);
#endif
}

void _sendstr(int fd, const char* prefix, const char* _buf) {
    std::string out = "";
    char *buf = strdup(_buf);
    for (char *str = buf, *save; ; str = NULL) {
        char* token = strtok_r(str, "\n", &save);
        if (token == NULL)
            break;
        out.append(prefix);
        out.append(token, strlen(token));
        out.append("\n");
    }
    free(buf);
#ifdef DEBUG
    std::cerr << out << std::flush;
#endif
    send(fd, out.c_str(), out.size(), SEND_FLAG);
}
void sendstr(int fd, const char* buf) {
    return _sendstr(fd, "", buf);
}
void sendstr(int fd, std::string buf) {
    return _sendstr(fd, "", buf.c_str());
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
