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
    ssize_t ssz;
    while (true) {
        std::cout << std::endl;

        bzero(&cliaddr, sizeof(cliaddr));
        clilen = sizeof(cliaddr);
        ssz = recvfrom(listenfd, buf, sizeof(buf), 0, (struct sockaddr*) &cliaddr, &clilen);
        if (ssz < 0) fail("recvfrom");
        if (VERBOSE >= 1) {
            char* info = sock_info(&cliaddr);
            std::cout << "[*] query from " << info << std::endl;
            free(info);
        }

        auto res = query({ buf, (size_t)ssz });
        if (VERBOSE >= 2) {
            Header header;
            header.parse(res.c_str());
            std::cout<< PAD{ 3, hexdump(res) } << PAD{ 6, header };
        }

        if (sendto(listenfd, res.c_str(), res.size(), 0, (struct sockaddr*) &cliaddr, clilen) < 0)
            fail("sendto(listenfd)");
    }
}

std::string Server::query(std::string qs) {
    Header header;
    header.parse(qs.c_str());
    if (VERBOSE >= 2)
        std::cout<< PAD{ 3, hexdump(qs) } << PAD{ 6, header };

    // TODO: handle multiple question
    assert(header.QDCOUNT == 1);
    auto q = header.question[0];
    
    for(auto dn = q.domain; dn.size(); dn.pop_back()) {
        try {
            auto rrs = config.get(dn).get(q);
            for(auto rr: rrs)
                header.answer.emplace_back(rr);
            return header.dump();
        } catch (...) {
        }
    }

    if (VERBOSE >= 1)
        std::cout << "[*] answer from " << config.forwardIP << std::endl;

    return forward(qs);
}

std::string Server::forward(std::string qs) {
    if (send(connfd, qs.c_str(), qs.size(), 0) < 0)
        fail("send(connfd)");
    char buf[2048];
    ssize_t ssz;
    ssz = recv(connfd, buf, sizeof(buf), 0);
    if (ssz < 0) fail("recv(connfd)");
    return { buf, (size_t)ssz };
}
