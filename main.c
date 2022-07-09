#define _GNU_SOURCE
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <stdint.h>
#include <stdbool.h>
#include "./png.h"

#ifndef MAX_CHUNK
#define MAX_CHUNK 1024
#endif

int main(int argc, char **argv) {
    const char *def_path = "media/maze.png";
    const char *path = argc > 1 ? argv[1] : def_path;
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

    close(fd);

    size_t offset = SIGNITURE_SIZE;

    chunk chunks[MAX_CHUNK];
    size_t chunks_size = 0;

    bool found_end = false;
    while(!found_end && offset < map_size && chunks_size < MAX_CHUNK) {

        chunk new_chunk;
        int bytesc = readChunk(data, map_size, &new_chunk, offset);
        if(bytesc == -1) {
            fprintf(stderr, "Error while reading chunk\n");
            return 1;
        }

        bool crit_chunk = !(new_chunk.type & 0x20);

        if(!crit_chunk) {
            printf("Ancillary chunk detected\n");
        }

        if(!new_chunk.valid) {
            fprintf(stderr, "Chunk CRC mismatch, corrupted chunk\n");
            fprintf(stderr, "Exiting\n");
            return 1;
        }

        chunks[chunks_size] = new_chunk;
        offset += CHUNK_SIZE(chunks[chunks_size]);

        switch(new_chunk.type) {
            case IHDR:
                printf("IHDR chunk (%zu)\n", chunks_size);
                break;

            case PLTE:
                printf("PLTE chunk (%zu)\n", chunks_size);
                break;

            case IDAT:
                printf("IDAT chunk (%zu)\n", chunks_size);
                break;

            case IEND:
                printf("IEND chunk (%zu)\n", chunks_size);
                found_end = true;
                break;
        }

        chunks_size++;
    }

    for(size_t idx = 0; idx < chunks_size; idx++) {
        /*printf("%u\n", chunks[idx].length);*/
        freeChunk(&chunks[idx]);
    }

    unmapFile(data, map_size);
    return 0;
}
