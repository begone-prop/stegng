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

    chunk chunks[MAX_CHUNK];
    size_t chunks_size;

    int status = parsePNG(path, chunks, MAX_CHUNK, &chunks_size);

    if(status == -1) {
        fprintf(stderr, "Failed to parse %s\n", path);
        return 1;
    }

    const char *out_path = "./out.png";
    const mode_t perm = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int ofd = open(out_path, O_WRONLY | O_CREAT | O_TRUNC, perm);
    size_t out_offset = 0;

    pwrite(ofd, validSigniture, SIGNITURE_SIZE, 0);
    out_offset += SIGNITURE_SIZE;

    for(size_t idx = 0; idx < chunks_size; idx++) {
        bool crit_chunk = !(chunks[idx].type & (1 << 0x1D));
        printChunk(chunks[idx]);
        if(!crit_chunk) {
            writeChunk(ofd, chunks[idx], out_offset);
            out_offset += CHUNK_SIZE(chunks[idx]);
        }
    }

    close(ofd);

    for(size_t idx = 0; idx < chunks_size; idx++) {
        freeChunk(&chunks[idx]);
    }

    return 0;
}
