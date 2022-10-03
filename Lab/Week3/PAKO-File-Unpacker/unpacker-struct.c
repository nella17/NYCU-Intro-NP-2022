#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>

#define chmax(x,y) \
    if ((x) < (y)) x = (y);

const uint32_t MAGIC = 0x4F4B4150;

void fail(char* s) {
    if (errno)
        perror(s);
    else
        fprintf(stderr, "%s: Unknown error", s);
    exit(-1);
}

bool is_dir(const char* path) {
    struct stat st;
    stat(path, &st);
    if (errno) {
        if (errno == ENOENT) {
            errno = 0;
            return false;
        }
        fail("is_dir");
    }
    return S_ISDIR(st.st_mode);
}

char* readstr(char* str, int fd, int32_t offset, size_t bytes) {
    if (str == NULL) {
        size_t align_bytes = ((bytes-1) / 8 + 1) * 8;
        str = malloc(align_bytes);
        memset(str, 0, align_bytes);
    }
    if (offset >= 0)
        lseek(fd, offset, SEEK_SET);
    read(fd, str, bytes);
    return str;
}

#define change_endian32(x) (x) = __builtin_bswap32(x)
#define change_endian64(x) (x) = __builtin_bswap64(x)

typedef union {
    uint64_t v64;
    uint8_t v8[8];
} CheckSum;

typedef struct {
    uint32_t magic;
    int32_t  off_str;
    int32_t  off_dat;
    uint32_t n_files;
} __attribute__((packed)) Header;

typedef struct {
    int32_t  off_file;
    uint32_t filesize;
    int32_t  off_data;
    CheckSum checksum;
} __attribute__((packed)) File_Meta;

typedef struct {
    File_Meta meta;
    char* filename;
    char* content;
} File;

bool check_checksum(File* f) {
    CheckSum* ary = (CheckSum*)f->content;
    CheckSum xor; xor.v64 = 0;
    size_t j = 0;
    for(; j+8 <= f->meta.filesize; j += 8)
        xor.v64 ^= ary[j/8].v64;
    for(; j+1 <= f->meta.filesize; j++)
        xor.v8[j%8] ^= ary[j/8].v8[j%8];
    bool checked = xor.v64 == f->meta.checksum.v64;
    return checked;
}

signed main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <src.pak> <dst>\n", argv[0]);
        return -1;
    }
    const char* src = argv[1];
    const char* dst = argv[2];

    int srcfd   = open(src, O_RDONLY);
    if (srcfd < 0) fail("open src");
    if (!is_dir(dst) && mkdir(dst, 0755) < 0)
        fail("mkdir fail");
    int dstfd   = open(dst, O_DIRECTORY);
    if (dstfd < 0) fail("open dst");

    off_t end_off = lseek(srcfd, 0, SEEK_END);
    if (end_off < 0) fail("error");
    size_t filesize = (size_t)end_off;
    char* filestr = readstr(NULL, srcfd, 0, filesize);

    Header header;
    memcpy(&header, filestr, sizeof(Header));
    if (header.magic != MAGIC) {
        fprintf(stderr, "Unknown magic %x\n", header.magic);
        return -1;
    }

    size_t max_filename = 0, max_filesize = 0;
    File files[ header.n_files ];
    for(size_t i = 0; i < header.n_files; i++) {
        memcpy
            (&files[i].meta,
            filestr+sizeof(Header)+i*sizeof(File_Meta),
            sizeof(File_Meta)
        );
        change_endian32(files[i].meta.filesize);
        change_endian64(files[i].meta.checksum.v64);
        files[i].filename = filestr + header.off_str + files[i].meta.off_file;
        files[i].content = filestr + header.off_dat + files[i].meta.off_data;
        chmax(max_filename, strlen(files[i].filename));
        chmax(max_filesize, files[i].meta.filesize);
    }

    for(size_t i = 0; i < header.n_files; i++) {
        bool checked = check_checksum(&files[i]);
        printf("%-*s %*d bytes 0x%016lx %s\n",
            (int)max_filename, files[i].filename,
            (int)log10(max_filesize)+1, files[i].meta.filesize,
            files[i].meta.checksum.v64,
            checked ? "" : "(checksum failed)"
        );
        if (checked) {
            int ffd = openat(dstfd, files[i].filename, O_CREAT | O_WRONLY, 0644);
            if (ffd < 0) {
                perror("openat");
            } else {
                write(ffd, files[i].content, files[i].meta.filesize);
                close(ffd);
            }
        }
    }

    close(srcfd);
    close(dstfd);

    return 0;
}
