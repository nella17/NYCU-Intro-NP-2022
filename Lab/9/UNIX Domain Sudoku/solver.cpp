#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <array>
#include <algorithm>

inline void fail(const char* s) {
    if (errno) perror(s);
    else fprintf(stderr, "%s: unknown error", s);
    exit(-1);
}

int connect(char* unix_path) {
    int sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sockfd < 0)
        fail("sockfd");

    struct sockaddr_un servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, unix_path);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        fail("connect");

    return sockfd;
}

using Board = std::array<std::array<size_t,9>,9>;

char n2c(size_t x) { return x ? char(x + '0') : '.'; }
size_t c2n(char c) { return c == '.' ? 0 : size_t(c - '0'); }

void print(const Board& mp) {
    for(size_t i = 0; i < 9; i++)
        for(size_t j = 0; j < 9; j++)
            printf("%c%c", n2c(mp[i][j]), " \n"[j+1==9]);
    printf("-----------\n");
}

bool valid(const Board& mp) {
    for(size_t i = 0; i < 9; i++) {
        std::array<size_t,10> cnt{};
        cnt.fill(0);
        for(size_t j = 0; j < 9; j++)
            cnt[ mp[i][j] ]++;
        cnt[0] = 0;
        auto mx = *std::max_element(cnt.begin(), cnt.end());
        if (mx > 1) return false;
    }
    for(size_t i = 0; i < 9; i++) {
        std::array<size_t,10> cnt{};
        cnt.fill(0);
        for(size_t j = 0; j < 9; j++)
            cnt[ mp[j][i] ]++;
        cnt[0] = 0;
        auto mx = *std::max_element(cnt.begin(), cnt.end());
        if (mx > 1) return false;
    }
    for(size_t i = 0; i < 9; i++) {
        size_t ix = i / 3, iy = i % 3;
        std::array<size_t,10> cnt{};
        cnt.fill(0);
        for(size_t j = 0; j < 9; j++) {
            size_t jx = j / 3, jy = j % 3;
            cnt[ mp[ix*3+jx][iy*3+jy] ]++;
        }
        cnt[0] = 0;
        auto mx = *std::max_element(cnt.begin(), cnt.end());
        if (mx > 1) return false;
    }
    return true;
}

bool dfs(Board& mp, size_t k) {
    if (k == 9*9) return true;
    size_t i = k / 9, j = k % 9;
    if (mp[i][j]) return dfs(mp, k+1);
    for(size_t x = 1; x <= 9; x++) {
        mp[i][j] = x;
        if (valid(mp) and dfs(mp, k+1))
            return true;
    }
    mp[i][j] = 0;
    return false;
}

Board solve(Board mp) {
    if (!dfs(mp, 0))
        fail("solve fail :(");
    return mp;
}

signed main(int argc, char *argv[]) {

    char unix_path[] = "/sudoku.sock";
    int connfd = connect(unix_path);

    write(connfd, "S\n", 2);

    char buf[1024];
    auto recv = [&]() {
        read(connfd, buf, sizeof(buf));
        write(1, buf, strlen(buf));
    };

    recv();

    Board init;
    for(size_t i = 0; i < 9; i++) for(size_t j = 0; j < 9; j++)
        init[i][j] = c2n(buf[4 + i*9 + j]);
    // print(init);

    Board done = solve(init);
    // print(done);

    for(size_t i = 0; i < 9; i++) for(size_t j = 0; j < 9; j++)
        if (init[i][j] == 0) {
            sprintf(buf, "V %lu %lu %c\n", i, j, n2c(done[i][j]));
            write(1, buf, strlen(buf));
            write(connfd, buf, strlen(buf));
            recv();
        }

    write(connfd, "C\n", 2);
    recv();

    return 0;
}
