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

    if(argc <= 1 || !strncmp(argv[1], "--help", 6)) {
        usage();
        return 0;
    }

    char *path = NULL;
    char *temp_path = NULL;
    char *out_path = NULL;
    bool strip_anci = false;
    bool print_chunks = false;
    bool inject_chunks_data = false;
    bool inject_chunks_file = false;
    bool extract_chunks = false;
    uint32_t ex_type;
    uint32_t inj_type = tEXt;
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
    while((opt = getopt(argc, argv, "i:o:j:spx:d:J:e:t:h")) != -1) {
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
                if(optarg[0] != '-' && !isdigit(optarg[0])) {
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

            case 'e':
                extract_chunks = true;
                /*ex_type =*/
                size_t e_size = strlen(optarg);

                if(e_size < 4) {
                    fprintf(stderr, "Type to extract must be 4 bytes (ex. tEXt)\n");
                    return 1;
                }

                for(size_t idx = 0; idx < 4; idx++) {
                    if(!isascii(optarg[idx])) {
                        fprintf(stderr, "Non ASCII character are not allowed\n");
                        return 1;
                    }
                }

                ex_type = *(uint32_t *)optarg;
                REVERSE(ex_type);
                break;

            case 't':
                (void)0;
                size_t t_size = strlen(optarg);

                if(t_size < 4) {
                    fprintf(stderr, "Type of custom chunk must be 4 bytes (ex. tEXt)\n");
                    return 1;
                }

                for(size_t idx = 0; idx < 4; idx++) {
                    if(!isascii(optarg[idx])) {
                        fprintf(stderr, "Non ASCII character are not allowed\n");
                        return 1;
                    }
                }

                inj_type = *(uint32_t *)optarg;

                if(IS_CRITICAL(inj_type)) fprintf(stderr, "Warning! Injecting data into chunk with critical bit turned on\n");


                REVERSE(inj_type);
                break;

            case 'h':
                usage();
                return 0;

            default:
                fprintf(stderr, "%s: Unknown option %s\n", program_invocation_short_name, optarg);
                usage();
                return 1;
        }
    }

    if(!path) {
        usage();
        return 0;
    }

    if(extract_chunks && (inject_chunks_file || inject_chunks_data)) {
        fprintf(stderr, "-e option if mutually exclusive with -J and -j option\n");
        goto final;
    }

    if(parsePNG(path, chunks, MAX_CHUNK, &chunks_size) == -1) {
        fprintf(stderr, "Failed to parse %s\n", path);
        return 1;
    }

    if(inject_chunks_data) {
        int ret = inject(chunks, &chunks_size, position, data_inj, strlen(data_inj), max_data_size, inj_type);
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


    if(extract_chunks) {
        int ofd;
        if(!out_path) {
            ofd = STDOUT_FILENO;
            if(isatty(ofd)) fprintf(stderr, "Writing binary data to stdout, this might confuse your terminal\n");
        }

        else {
            const mode_t perm = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
            ofd = open(out_path, O_WRONLY | O_CREAT | O_TRUNC, perm);
            if(ofd == -1) {
                fprintf(stderr, "Failed to open %s: %s\n", out_path, strerror(errno));
                goto final;
            }
        }

        for(size_t idx = 0; idx < chunks_size; idx++) {
            if(chunks[idx].type == ex_type) {
                if(chunks[idx].length == 0) continue;
                if(write(ofd, chunks[idx].data, chunks[idx].length) == -1) {
                    fprintf(stderr, "Failed to write chunk data\n");
                    goto final;
                }
            }
        }
        close(ofd);
        goto final;
    }

    if(print_chunks) {
        for(size_t idx = 0; idx < chunks_size; idx++) {
            if(strip_anci && !IS_CRITICAL(chunks[idx].type)) continue;
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
