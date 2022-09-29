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

const int MAX_SIZE = 1024;

void check_errno(char* s) {
    if (errno) {
        perror(s);
        exit(-1);
    }
}

char* readstr(int fd, int offset, int bytes) {
    char* str = malloc(bytes+0x20);
    memset(str, 0, bytes+0x20);
    if (offset >= 0)
        lseek(fd, offset, SEEK_SET);
    read(fd, str, bytes);
    return str;
}

uint64_t readint(int fd, int offset, int bytes, _Bool little) {
    assert(bytes == 4 || bytes == 8);
    char* str = readstr(fd, offset, bytes);
    uint64_t r = 0;
    if (little)
        r = *(uint64_t*)str;
    else
        for(int i = 0; i < bytes; i++)
            r = r * 0x100 + (uint8_t)str[i];
    free(str);
    return r;
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

    lseek(fd, 4, SEEK_SET);
    uint32_t str_sec    = readint(fd, -1, 4, 1);
    uint32_t cont_sec   = readint(fd, -1, 4, 1);
    uint32_t nfiles     = readint(fd, -1, 4, 1);
    // printf("%x %x %x\n", str_sec, cont_sec, nfiles);

    for(int i = 0; i < nfiles; i++) {
        lseek(fd, 0x10 + i*0x14, SEEK_SET);
        uint32_t fn_off     = readint(fd, -1, 4, 1);
        uint32_t fsize      = readint(fd, -1, 4, 0);
        uint32_t cont_off   = readint(fd, -1, 4, 1);
        uint64_t checksum   = readint(fd, -1, 8, 0);
        // printf("%x %x %x %lx\n", fn_off, fsize, cont_off, checksum);
        char* file  = readstr(fd, str_sec + fn_off, MAX_SIZE);
        char* cont  = readstr(fd, cont_sec + cont_off, fsize);
        uint64_t* ary = (uint64_t*)cont;
        uint64_t xor = 0;
        for(int j = 0; j*8 < fsize; j++)
            xor ^= ary[j];
        _Bool checked = xor == checksum;
        printf("%s: %d bytes %lx %s\n",
            file, fsize, checksum, checked ? "" : "(checksum failed)");
        if (!checked) {
            /*
            printf("%lx\n%lx\n%lx\n", checksum, xor, checksum^xor);
            if (fsize <= 8*10) {
                puts(cont);
                for(int j = 0; j*8 < fsize; j++)
                    printf("%lx ", ary[j]);
            }
            puts("\n");
            //*/
        } else {
            int ffd = openat(dirfd, file, O_CREAT | O_WRONLY, 0644);
            if (ffd < 0) perror("openat");
            write(ffd, cont, fsize);
            // printf("%d %s\n", ffd, file);
            close(ffd);
        }
        free(file);
        free(cont);
    }

    close(fd);
    close(dirfd);

    return 0;
}
