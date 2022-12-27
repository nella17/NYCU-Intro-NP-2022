#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

#include "config.hpp"
#include "utils.hpp"

Config::Config(const char config_path_s[]) {
    fs::path config_path(config_path_s);
    fs::path config_dir = config_path.parent_path();

    auto config = fopen(config_path.c_str(), "r");
    if (!config) fail("open(config)");

    char ip[20];
    fscanf(config, "%19s", ip);
    printf("[config] forwardIP: '%s'\n", ip);
    if (inet_pton(AF_INET, ip, &forwardIP) <= 0)
        fail("inet_pton(forwardIP)");

    char domain[256], path[512];
    while (fscanf(config, " %255[^,],%511s", domain, path) != EOF) {
        fs::path zone_path = config_dir / path;
        printf("[config] zone '%s' -> '%s'\n", domain, zone_path.c_str());
        auto zone_file = fopen(zone_path.c_str(), "r");
        if (!zone_file) fail("open(zone)");

        char buf[1024];
        fgets(buf, 1024, zone_file);
        char* save;
        strtok_r(buf, "\r\n", &save);
        if (strcmp(domain, buf) != 0) {
            fprintf(stderr, "[config] '%s' != '%s'\n", domain, buf);
            continue;
        }

        while (fgets(buf, 1024, zone_file)) {
            char name[256], type[10], clas[5], rdata[512];
            uint32_t ttl;
            sscanf(buf, " %255[^,],%d,%4[^,],%9[^,],%511[^\r\n]", name, &ttl, clas, type, rdata);
            // printf("[config] [%s] '%s' '%d' '%s' '%s' '%s'\n", domain, name, ttl, clas, type, rdata);
            Record rr(s2dn(name) + s2dn(domain), s2type(type), s2clas(clas), ttl, rdata);
            std::cout << "[config] " << rr << std::endl;
        }
        fclose(zone_file);
    }

    fclose(config);
}
