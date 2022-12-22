#include "utils.hpp"

signed main(int argc, char* argv[]) {
    if (argc != 4)
        return -fprintf(stderr, "Usage: %s <path-to-store-files> <total-number-of-files> <broadcast-address>", argv[0]);

    setup();

    auto path = argv[1];
    auto total = (uint32_t)atoi(argv[2]);
    auto broadip = argv[3];

    puts(broadip);

    int sockfd = create_sock();

    struct sockaddr_in broad;
    bzero(&broad, sizeof(broad));
    broad.sin_family = AF_INET;
    if (inet_pton(AF_INET, broadip, &broad.sin_addr) <= 0)
        fail("inet_pton");

    std::vector<file_t> files(total);
    std::map<uint32_t, data_t> data_map{};
    size_t total_bytes = 0;
    for(uint32_t idx = 0; idx < total; idx++) {
        char filename[256];
        sprintf(filename, "%s/%06d", path, idx);
        int filefd = open(filename, O_RDONLY);
        if (filefd < 0) fail("open");
        auto size = (uint32_t)lseek(filefd, 0, SEEK_END);
        auto data = new char[size];
        lseek(filefd, 0, SEEK_SET);
        read(filefd, data, size);
        close(filefd);
        files[idx] = {
            .filename = idx,
            .size = size,
            .init = {
                .filename = idx,
                .filesize = size,
            },
            .data = data,
        };
        printf("[cli] read '%s' (%u bytes)\n", filename, size);
        total_bytes += size;
    }

    auto orig_size = total * sizeof(init_t) + total_bytes;
    auto orig_data = new char[orig_size];
    {
        auto ptr = orig_data;
        for(auto& file: files) {
            memcpy(ptr, &file.init, sizeof(init_t));
            ptr += sizeof(init_t);
            memcpy(ptr, file.data, file.size);
            ptr += file.size;
        }
    }

    data_t zero{
        .data_size = orig_size
    };
    data_map.emplace(0, data_t { .data_size = sizeof(data_t), .data = (char*)&zero });
    for(size_t i = 0; i * DATA_SIZE < orig_size; i++) {
        size_t offset = i * DATA_SIZE;
        data_map.emplace(
            i+1,
            data_t{
                .data_size = std::min(DATA_SIZE, orig_size - offset),
                .data = orig_data + offset,
            }
        );
    }

    fprintf(stderr, "[cli] total %lu bytes\n", total_bytes);

    pkt_t s_res;
    auto& donebit = *(std::bitset<DATA_SIZE*8>*)&s_res.data;

    auto read_resps = [&]() {
        pkt_t res;
        while (recv(sockfd, &res, sizeof(res), MSG_WAITALL) == sizeof(res)) {
            if (checksum(&res) != res.header.checksum) {
                printf("[cli] error resp\n");
                // dump_hdr(&res);
                continue;
            }
            s_res = res;
            printf("[cli] update bitset %lu\n", donebit.count());
            if (res.header.sess_seq == UINT32_MAX) {
                printf("[cli] sent done for (%lu bytes)\n", orig_size);
                // usleep(100);
                exit(0);
                continue;
            }
        }
    };

    int r = 0;
    while (!data_map.empty()) {
        fprintf(stderr, "[cli] %d: %lu data packets left\n", ++r, data_map.size());
        for(auto [key, data] : data_map) if (!donebit[key])  {
            pkt_t pkt;
            bzero(&pkt, sizeof(pkt));
            pkt.header.sess_seq = key;
            memcpy(pkt.data, data.data, data.data_size);
            send(sockfd, pkt, broad);
            usleep(500);
        }
        read_resps();
        for (size_t i = 0; i < DATA_SIZE*8; i++)
            if (donebit[i]) data_map.erase((uint32_t)i);
    }


}
