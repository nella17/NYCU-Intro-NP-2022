#pragma once

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

constexpr int SEND_FLAG = MSG_NOSIGNAL | MSG_DONTWAIT;

void fail(const char* s);

ssize_t sendstr(int fd, const char* buf);
ssize_t sendstr(int fd, std::string buf);

char* sock_info(const struct sockaddr_in* sock);
