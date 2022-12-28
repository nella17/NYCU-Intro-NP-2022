#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include <iostream>

#include "config.hpp"
#include "utils.hpp"

Config::Config(const char config_path_s[]) {
    fs::path config_path(config_path_s);
    fs::path config_dir = config_path.parent_path();

    auto config = fopen(config_path.c_str(), "r");
    if (!config) fail("open(config)");

    fscanf(config, "%19s", forwardIP);
    printf("[config] forwardIP: '%s'\n", forwardIP);

    char domain[256], path[512];
    while (fscanf(config, " %255[^,],%511s", domain, path) != EOF) {
        fs::path zone_path = config_dir / path;
        printf("[config] zone '%s' -> '%s'\n", domain, zone_path.c_str());

        DN dn = s2dn(domain);
        zones.try_emplace(dn, dn, zone_path);
    }

    fclose(config);
}

bool Config::has(DN domain) const {
    return zones.count(domain) > 0;
}

const Zone& Config::get(DN domain) const {
    return zones.find(domain)->second;
}
