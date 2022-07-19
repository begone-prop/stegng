#define _GNU_SOURCE
#define _ISOC99_SOURCE

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include "./png.h"

int main(int argc, char **argv) {
    char *path = NULL;
    char *temp_path = NULL;
    const char *out_path = NULL;
    bool strip_anci = false;
    bool print_chunks = false;
    bool inject_chunks_data = false;
    bool inject_chunks_file = false;
    char *data_inj = NULL;
    char *file_inj = NULL;
    struct stat sbuff;
    long int position_def = -1;
    long int position = position_def;
    size_t max_data_size_def = 128000;
    size_t max_data_size = max_data_size_def;

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    chunk chunks[MAX_CHUNK];
    size_t chunks_size;

    int opt;
    while((opt = getopt(argc, argv, "i:o:j:spx:d:J:")) != -1) {
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

            case 'j':
                inject_chunks_data = true;
                data_inj = optarg;
                break;

            case 'J':
                inject_chunks_file = true;
                file_inj = optarg;
                break;

            case 'x':
                if(optarg[0] != '-' || !isdigit(optarg[0])) {
                    fprintf(stderr, "Invalid number format %s\n", optarg);
                }

                position = strtol(optarg, NULL, 10);
                break;

            case 'd':
                if(!isdigit(optarg[0])) {
                    fprintf(stderr, "Invalid number format %s\n", optarg);
                }

                int val = strtol(optarg, NULL, 10);
                if(val == 0) val = UINT32_MAX;
                max_data_size = val;
                break;

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

    if(inject_chunks_data) {
        int ret = inject(chunks, &chunks_size, position, data_inj, strlen(data_inj), max_data_size, tEXt);
        if(ret == -1) {
            fprintf(stderr, "Failed to inject chunks\n");
            goto final;
        }
    }

    if(inject_chunks_file) {
        int fd = open(file_inj, O_RDONLY);
        if(fd == -1) {
            fprintf(stderr, "Failed to open %s: %s\n", file_inj, strerror(errno));
            goto final;
        }

        if(stat(file_inj, &sbuff) == -1) {
            fprintf(stderr, "%s: %s\n", program_invocation_short_name, strerror(errno));
            return 1;
        }

        if(!S_ISREG(sbuff.st_mode)) {
            fprintf(stderr, "%s: %s is not a regular file\n", program_invocation_short_name, file_inj);
            return 1;
        }
        size_t addr_size;
        void *addr = mapFile(fd, &addr_size);
        if(!addr) {
            fprintf(stderr, "Failed to map %s\n", file_inj);
            goto final;
        }

        close(fd);
        int ret = inject(chunks, &chunks_size, position, addr, addr_size, max_data_size, tEXt);

        if(ret == -1) {
            fprintf(stderr, "Failed to inject chunks\n");
            goto final;
        }

        unmapFile(addr, addr_size);
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

    final:
    for(size_t idx = 0; idx < chunks_size; idx++) {
        freeChunk(&chunks[idx]);
    }

    return 0;
}
