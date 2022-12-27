#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "utils.hpp"

void fail(const char* s) {
    if (errno) perror(s);
    else fprintf(stderr, "%s: unknown error", s);
#ifdef DEBUG
    exit(-1);
#endif
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

std::string inet_pton(int af, std::string data) {
    size_t sz = 0;
    switch (af) {
    case AF_INET:
        sz = sizeof(struct in_addr);
        break;
    case AF_INET6:
        sz = sizeof(struct in6_addr);
        break;
    }
    if (!sz) return nullptr;
    char buf[sz];
    bzero(buf, sz);
    assert(inet_pton(af, data.c_str(), buf) == 1);
    return { buf, sz };
}
