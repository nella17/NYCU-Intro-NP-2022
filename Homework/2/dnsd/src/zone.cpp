#include <iostream>
#include <fstream>
#include <string>

#include "zone.hpp"
#include "utils.hpp"

Zone::Zone(DN _domain, fs::path zone_path): domain(_domain) {
    std::ifstream in;
    in.open(zone_path);

    std::string line;
    auto readline = [&]() -> std::ifstream& {
        std::getline(in, line);
        if (line.size() and line.back() == '\r')
            line.pop_back();
        return in;
    };

    readline();
    if (s2dn(line) != domain) {
        std::cerr << "[config] " << line << " != " << domain << std::endl;
        exit(-1);
    }

    while (readline()) {
        char name[256], type[10], clas[5], rdata[512];
        uint32_t ttl;
        sscanf(line.c_str(), " %255[^,],%d,%4[^,],%9[^,],%511[^\r\n]", name, &ttl, clas, type, rdata);
        // printf("[config] [%s] '%s' '%d' '%s' '%s' '%s'\n", domain, name, ttl, clas, type, rdata);
        Record rr(domain + s2dn(name), v2type(type), v2clas(clas), ttl, rdata);
        add(rr);
        if (VERBOSE >= 1)
            std::cout << "   " << rr << std::endl;
        if (VERBOSE >= 2)
            std::cout << PAD{ 6, hexdump(rr.rdata()) };
    }

    in.close();
}

bool Zone::add(Record rr) {
    if (not(rr.domain % domain)) {
        std::cerr << "Add " << rr.domain << " not in " << domain << std::endl;
        exit(-1);
    }
    rrs[rr.key()].emplace_back(rr);
    return true;
}

Records Zone::get(Question q) const {
    if (not(q.domain % domain)) {
        std::cerr << "Get " << q.domain << " not in " << domain << std::endl;
        exit(-1);
    }

    auto it = rrs.find(q.key());
    if (it != rrs.end())
        return it->second;

}
