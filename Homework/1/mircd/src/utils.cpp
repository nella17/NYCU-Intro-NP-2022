#include "utils.hpp"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void fail(const char* s) {
    if (errno) perror(s);
    else fprintf(stderr, "%s: unknown error", s);
    exit(-1);
}

ssize_t sendstr(int fd, const char* buf) {
    return send(fd, buf, strlen(buf), SEND_FLAG);
}
ssize_t sendstr(int fd, std::string buf) {
    return send(fd, buf.c_str(), buf.size(), SEND_FLAG);
}

char* sock_info(const struct sockaddr_in* sock) {
    char buf[64], host[16];
    sprintf(buf, "%s:%d", inet_ntop(AF_INET, &sock->sin_addr, host, 16), ntohs(sock->sin_port));
    return strdup(buf);
}

