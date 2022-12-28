#include <unistd.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <iostream>

#include "server.hpp"
#include "header.hpp"
#include "utils.hpp"

Server::Server(uint16_t listenport, const char config_path[]): config(config_path) {
	listenfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (listenfd < 0) fail("listenfd");

    int on = 1;
    if ((on = 1) && setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof (on)) < 0) {
        fail("setsockopt(SO_REUSEPORT)"); }

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(listenport);

	if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        fail("bind");

    connfd = connect(config.forwardIP, 53);
}

void Server::interactive() {
    struct sockaddr_in cliaddr;
    socklen_t clilen;
    char buf[2048];
    Header header;
    size_t sz;
    ssize_t ssz;
    while (true) {
        std::cout << std::endl;

        bzero(&cliaddr, sizeof(cliaddr));
        clilen = sizeof(cliaddr);
        if ((ssz = recvfrom(listenfd, buf, sizeof(buf), 0, (struct sockaddr*) &cliaddr, &clilen)) < 0) fail("recvfrom");
        sz = (size_t)ssz;
        if (VERBOSE >= 1) {
            char* info = sock_info(&cliaddr);
            std::cout << "[*] query from " << info << std::endl;
            free(info);
        }

        header.parse(buf);
        if (VERBOSE >= 2)
            std::cout<< HEX{ 3,  { buf, sz } } << header;

        if (send(connfd, buf, sz, 0) < 0)
            fail("send(connfd)");
        if ((ssz = recv(connfd, buf, sizeof(buf), 0)) < 0) fail("recv(connfd)");
        sz = (size_t)ssz;

        if (sendto(listenfd, buf, sz, 0, (struct sockaddr*) &cliaddr, clilen) < 0)
            fail("sendto(listenfd)");
        if (VERBOSE >= 1)
            std::cout << "[*] answer from " << config.forwardIP << std::endl;

        header.parse(buf);
        if (VERBOSE >= 2)
            std::cout<< HEX{ 3,  { buf, sz } } << header;
    }
}
