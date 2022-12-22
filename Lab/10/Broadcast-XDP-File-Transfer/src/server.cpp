#include "utils.hpp"

using DATA_MAP_T = std::map<uint32_t, std::array<char,DATA_SIZE>>;

signed main(int argc, char* argv[]) {
    if (argc != 4)
        return -fprintf(stderr, "Usage: %s <path-to-read-files> <total-number-of-files> <broadcast-address>", argv[0]);

    setup();

    auto path = argv[1];
    auto total = (uint32_t)atoi(argv[2]);
    auto broadip = argv[3];

    struct sockaddr_in broad;
    bzero(&broad, sizeof(broad));
    broad.sin_family = AF_INET;
    if (inet_pton(AF_INET, broadip, &broad.sin_addr) <= 0)
        fail("inet_pton");

    int sockfd = create_sock();

    struct sockaddr_in src;
    bzero(&src, sizeof(src));
    socklen_t len = sizeof(src);

    pkt_t send_pkt{
        .header {
            .sess_seq = 0
        }
    };
    auto& recvbit = *(std::bitset<DATA_SIZE*8>*)&send_pkt.data;

    auto last_send = std::chrono::steady_clock::now();
    uint32_t orig_size = UINT_MAX, recv_size = 0;
    DATA_MAP_T data_map;
    pkt_t recv_pkt;

    while (orig_size == UINT_MAX or recv_size < orig_size) {
        if (recvfrom(sockfd, &recv_pkt, sizeof(recv_pkt), 0, (sockaddr*)&src, &len)) {
            if (checksum(&recv_pkt) != recv_pkt.header.checksum) {
                printf("[/] error recv\n");
                continue;
            }
            // session initialization
            auto data_seq = recv_pkt.header.sess_seq;
            recvbit[data_seq] = 1;
            if (data_seq == 0) {
                if (orig_size == UINT_MAX) {
                    data_t tmp;
                    memcpy(&tmp, recv_pkt.data, sizeof(data_t));
                    orig_size = (uint32_t)tmp.data_size;
                    printf("[/] [ChunkID=%d] initiation %d bytes\n", data_seq, orig_size);
                }
            } else {
                if (!data_map.count(data_seq)) {
                    auto& data_frag = data_map[data_seq];
                    memcpy(&data_frag, recv_pkt.data, DATA_SIZE);
                    recv_size += DATA_SIZE;
                }
            }
        }

        auto now = std::chrono::steady_clock::now();
        if (now - last_send > std::chrono::milliseconds(200)) {
            last_send = now;
            fprintf(stderr, "[/] send %lu ack\n", recvbit.count());
            send(sockfd, send_pkt, broad);
        }
    }

    auto orig_data = new char[orig_size];
    for(auto [seq, data_frag]: data_map)
        if (seq)
            memcpy(orig_data + (seq-1) * DATA_SIZE, &data_frag, DATA_SIZE);

    int cnt = 0;
    for(auto ptr = orig_data; ptr < orig_data + orig_size; cnt++) {
        auto init = (init_t*)ptr;
        ptr += sizeof(init_t);
        auto filesize = init->filesize;

        char filename[100];
        sprintf(filename, "%s/%06d", path, init->filename);
        printf("[/] File saved to %s (%d bytes)\n", filename, filesize);
        int fp = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fp == -1) fail("open");
        write(fp, ptr, filesize);
        ptr += filesize;
        close(fp);
    }

    send_pkt.header.sess_seq = UINT32_MAX;
    while (true) {
        send(sockfd, send_pkt, broad);
        usleep(500);
    }

    pkt_t packet;
    while (true) {
        bzero(&packet, sizeof(packet));
        long res = recvfrom(sockfd, &packet, sizeof(packet), 0, (sockaddr*)&src, &len);
        printf("[s] %ld %ld\n", res, write(1, packet.data, DATA_SIZE));
        if (res < 0) perror("[c] recvfrom");
    }
}
