#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>

const size_t MAX_SIZE = 256;

void check_errno(char* s) {
    if (errno) {
        perror(s);
        exit(-1);
    }
}

char* readstr(char* str, int fd, int32_t offset, size_t bytes) {
    if (str == NULL) {
        size_t align_bytes = (bytes / 8 + 1) * 8;
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
    uint64_t checksum;
} __attribute__((packed)) File_Meta;

typedef struct {
    File_Meta meta;
    char* filename;
    char* content;
} File;

_Bool check_checksum(File* f) {
    uint64_t* ary = (uint64_t*)f->content;
    uint64_t xor = 0;
    for(size_t j = 0; j*8 < f->meta.filesize; j++)
        xor ^= ary[j];
    _Bool checked = xor == f->meta.checksum;
    return checked;
}

signed main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <src.pak> <dst>\n", argv[0]);
        return -1;
    }
    const char* src = argv[1];
    const char* dst = argv[2];

    int fd      = open(src, O_RDONLY);
    check_errno("open src");
    if (mkdir(dst, 0755)) {
        if (errno != EEXIST)
            check_errno("mkdir fail");
        errno = 0;
    }
    int dirfd   = open(dst, O_DIRECTORY);
    check_errno("open dst");

    Header header;
    readstr((char*)&header, fd, -1, sizeof(Header));
    if (header.magic != 0x4F4B4150) {
        fprintf(stderr, "Unknown magic %x\n", header.magic);
        return -1;
    }

    File files[ header.n_files ];
    for(size_t i = 0; i < header.n_files; i++) {
        readstr((char*)&files[i].meta, fd, -1, sizeof(File_Meta));
        change_endian32(files[i].meta.filesize);
        change_endian64(files[i].meta.checksum);
    }

    for(size_t i = 0; i < header.n_files; i++) {
        files[i].filename = readstr(NULL, fd, header.off_str + files[i].meta.off_file, MAX_SIZE);
        files[i].content = readstr(NULL, fd, header.off_dat + files[i].meta.off_data, files[i].meta.filesize);
    }

    for(size_t i = 0; i < header.n_files; i++) {
        _Bool checked = check_checksum(&files[i]);
        printf("%s: %d bytes %lx %s\n",
            files[i].filename, files[i].meta.filesize, files[i].meta.checksum,
            checked ? "" : "(checksum failed)");
        if (checked) {
            int ffd = openat(dirfd, files[i].filename, O_CREAT | O_WRONLY, 0644);
            if (ffd < 0) perror("openat");
            write(ffd, files[i].content, files[i].meta.filesize);
            close(ffd);
        }
    }

    for(size_t i = 0; i < header.n_files; i++) {
        free(files[i].filename);
        free(files[i].content);
    }
    close(fd);
    close(dirfd);

    return 0;
}
