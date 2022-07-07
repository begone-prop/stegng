#define _GNU_SOURCE
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/mman.h>

#ifndef MAX_CHUNK
#define MAX_CHUNK 1024
#endif

#define SIGNITURE_SIZE 8
static const uint8_t validSigniture[] = {137, 80, 78, 71, 13, 10, 26, 10};

#define HAS_VALID_SIGNITURE(A) (memcmp((A), (validSigniture), SIGNITURE_SIZE) == 0 ? true : false)

void *mapFile(int fd, size_t *map_size) {
    struct stat sbuf;
    if(fstat(fd, &sbuf) == 1) return NULL;
    void *data = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if(data == MAP_FAILED) return NULL;

    if(map_size) *map_size = sbuf.st_size;
    return data;
}

int main(int argc, char **argv) {
    const char *path = "media/maze.png";
    int fd = open(path, O_RDONLY);
    if(fd == -1) err(1, "Failed to open %s", path);

    static uint8_t sig_buff[SIGNITURE_SIZE];
    ssize_t bytesr = pread(fd, sig_buff, SIGNITURE_SIZE, 0);
    if(bytesr == -1) err(1, "Failed to read from %s", path);

    if(!HAS_VALID_SIGNITURE(sig_buff)) {
        fprintf(stderr, "Not a valid signiture on file %s\n", path);
        return 1;
    }

    size_t map_size = 0;
    void *data = mapFile(fd, &map_size);
    if(!data) {
        fprintf(stderr, "Failed to map file %s\n", path);
        return 1;
    }

    size_t offset = SIGNITURE_SIZE;

    munmap(data, map_size);
    return 0;
}
