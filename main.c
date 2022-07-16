#define _GNU_SOURCE
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
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
    bool inject_chunks = false;
    char *file_inj = NULL;
    static const char *keyword_def = "data";
    const size_t keyword_def_size = strlen(keyword_def);
    struct stat sbuff;

    chunk chunks[MAX_CHUNK];
    size_t chunks_size;

    int opt;
    while((opt = getopt(argc, argv, "i:o:j:sp")) != -1) {
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

            case 'j': {
                inject_chunks = true;
                file_inj = optarg;

                break;
            }

            default:
                fprintf(stderr, "%s: Unknown option %s", program_invocation_short_name, optarg);
                return 1;
        }
    }

    if(!path) {
        fprintf(stderr, "No input file...\n"
            "Usage: %s -i <file> -o <file>\n", program_invocation_short_name);
        return 0;
    }

    if(parsePNG(path, chunks, MAX_CHUNK, &chunks_size) == -1) {
        fprintf(stderr, "Failed to parse %s\n", path);
        return 1;
    }


    if(inject_chunks) {

        int fd = open(file_inj, O_RDONLY);

        if(fd == -1) {
            fprintf(stderr, "Failed to open %s: %s\n", file_inj, strerror(errno));
            return 1;
        }

        size_t inj_size;
        void *inj = mapFile(fd, &inj_size);
        close(fd);

        if(!inj) {
            fprintf(stderr, "Failed to map file %s\n", file_inj);
            return 1;
        }

        size_t data_size_bytes = inj_size;
        size_t injected_data_size = data_size_bytes + keyword_def_size + 1;

        if(injected_data_size >= UINT32_MAX) {
            fprintf(stderr, "Size of data (%zu) is bigger than maxium chunk data size (UINT32: %u)\n", injected_data_size, UINT32_MAX);
            return 1;
        }

        printf("Injected data size %zu bytes\n", injected_data_size);

        chunk new_chunk;
        void *inj_data = malloc(injected_data_size);
        new_chunk.type = tEXt;
        new_chunk.length = injected_data_size;

        memcpy((uint8_t *)inj_data, keyword_def, keyword_def_size);
        *(char *)((uint8_t*)inj_data + keyword_def_size) = '\0';
        memcpy((uint8_t *)inj_data + keyword_def_size + 1, inj, inj_size);
        new_chunk.data = inj_data;

        uint32_t crc = calcChunkCRC(new_chunk);

        new_chunk.crc = crc;
        new_chunk.valid = true;
        chunk end = chunks[chunks_size - 1];
        chunks[chunks_size - 1] = new_chunk;
        chunks[chunks_size] = end;
        chunks_size++;

        unmapFile(inj, inj_size);
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
