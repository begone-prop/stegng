#define _GNU_SOURCE
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include "./png.h"

#ifndef MAX_CHUNK
#define MAX_CHUNK 1024
#endif

int main(int argc, char **argv) {
    char *path = NULL;
    char *temp_path = NULL;
    const char *out_path = NULL;
    bool strip_anci = false;
    bool print_chunks = false;
    struct stat sbuff;

    chunk chunks[MAX_CHUNK];
    size_t chunks_size;

    int opt;
    while((opt = getopt(argc, argv, "i:o:sp")) != -1) {
        switch(opt) {
            case 'i':
                (void)0;

                temp_path = optarg;
                if(stat(temp_path, &sbuff) == -1) {
                    fprintf(stderr, "%s: %s\n", program_invocation_short_name, strerror(errno));
                    return 1;
                }

                if(!S_ISREG(sbuff.st_mode)) {
                    fprintf(stderr, "%s: %s is not a regular file\n", program_invocation_short_name, temp_path);
                    return 1;
                }

                path = temp_path;
                break;

            case 'o':
                (void)0;

                temp_path = optarg;
                int retval = stat(temp_path, &sbuff);

                if(retval == -1 && errno != ENOENT) {
                    fprintf(stderr, "%s: %s\n", program_invocation_short_name, strerror(errno));
                    return 1;
                }

                out_path = temp_path;

                break;

            case 's':
                strip_anci = true;
                break;

            case 'p':
                print_chunks = true;
                break;

            default:
                fprintf(stderr, "%s: Unknown option %s", program_invocation_short_name, optarg);
                return 1;
        }
    }

    if(!path) {
        fprintf(stderr, "No input file...\n");
        fprintf(stderr, "Usage: %s -i <file> -o <file>\n", program_invocation_short_name);
        return 0;
    }

    if(parsePNG(path, chunks, MAX_CHUNK, &chunks_size) == -1) {
        fprintf(stderr, "Failed to parse %s\n", path);
        return 1;
    }

    if(print_chunks) {
        for(size_t idx = 0; idx < chunks_size; idx++) {
            if(strip_anci && !IS_CRITICAL(chunks[idx])) continue;
            printChunk(chunks[idx]);
        }
    }

    if(out_path) {
        int wstatus = writePNG(out_path, chunks, chunks_size, strip_anci);

        if(wstatus == -1) {
            fprintf(stderr, "Failed to write chunks to %s\n", out_path);
            return 1;
        }
    }

    for(size_t idx = 0; idx < chunks_size; idx++) {
        freeChunk(&chunks[idx]);
    }

    return 0;
}
