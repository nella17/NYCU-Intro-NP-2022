#pragma once

#include <cstdlib>
#include <stdio.h>
#include <errno.h>
#include <cstddef>
#include <stdint.h>
#include <sys/socket.h>

#include "header.hpp"

const uint32_t MOD_ADLER = 65521;

inline void fail(const char* s) {
    if (errno) perror(s);
    else fprintf(stderr, "%s: unknown error", s);
    exit(-1);
}

inline void setvbufs() {
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
}

inline uint32_t adler32(const void *_data, size_t len)
/*
    where data is the location of the data in physical memory and
    len is the length of the data in bytes
*/
{
    auto data = (const uint8_t*)_data;
    uint32_t a = 1, b = 0;
    size_t index;

    // Process each byte of the data in order
    for (index = 0; index < len; ++index)
    {
        a = (a + data[index]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }

    return (b << 16) | a;
}

inline void dump_sender_hdr(const struct sender_hdr_t* hdr) {
    fprintf(stderr, "[*] Checksum, seq=%d, recv_chk=%x, my_chk=%x\n",
        hdr->data_seq, hdr->checksum, adler32(hdr->data, DATA_SIZE)
    );
    // dump data as hex
    fprintf(stderr, "[*] Data: ");
    for (size_t i = 0; i < DATA_SIZE; i++) {
        fprintf(stderr, "%02x ", (uint8_t)hdr->data[i]);
    }
    fprintf(stderr, "\n");
}