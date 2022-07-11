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
    const char *out_path = "./out.png";

    chunk chunks[MAX_CHUNK];
    size_t chunks_size;

    int status = parsePNG(path, chunks, MAX_CHUNK, &chunks_size);

    if(status == -1) {
        fprintf(stderr, "Failed to parse %s\n", path);
        return 1;
    }

    int wstatus = writePNG(out_path, chunks, chunks_size);

    if(wstatus == -1) {
        fprintf(stderr, "Failed to write chunks to %s\n", out_path);
        return 1;
    }

    for(size_t idx = 0; idx < chunks_size; idx++) {
        freeChunk(&chunks[idx]);
    }

    return 0;
}
