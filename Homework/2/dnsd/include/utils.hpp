#pragma once

#include <sys/socket.h>

void fail(const char* s);

char* sock_info(const struct sockaddr_in* sock);
char* sock_host(const struct sockaddr_in* sock);
