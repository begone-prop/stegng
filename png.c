#define _XOPEN_SOURCE 500

#include "./png.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>

static unsigned long update_crc(unsigned long, unsigned char *, int);
static void make_crc_table(void);

int writePNG(const char *path, chunk *chunks, size_t chunks_size, bool crit_only) {
    const mode_t perm = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int ofd = open(path, O_WRONLY | O_CREAT | O_TRUNC, perm);
    if(ofd == -1) return -1;
    size_t out_offset = 0;

    if(pwrite(ofd, validSigniture, SIGNITURE_SIZE, 0) == -1)
        return -1;

    out_offset += SIGNITURE_SIZE;

    for(size_t idx = 0; idx < chunks_size; idx++) {
        if(crit_only && !IS_CRITICAL(chunks[idx])) continue;

        if(writeChunk(ofd, chunks[idx], out_offset) == -1) {
            close(ofd);
            return -1;
        }
        out_offset += CHUNK_SIZE(chunks[idx]);
    }

    close(ofd);
    return 1;
}

int parsePNG(const char *path, chunk *chunk_buffer, size_t cb_max, size_t *cb_size) {
    int fd = open(path, O_RDONLY);
    if(fd == -1) return -1;

    static uint8_t sig_buff[SIGNITURE_SIZE];
    ssize_t bytesr = pread(fd, sig_buff, SIGNITURE_SIZE, 0);
    if(bytesr == -1) return -1;

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

    size_t chunks_size = 0;

    bool found_end = false;
    while(!found_end && offset < map_size && chunks_size < cb_max) {

        chunk new_chunk;
        int bytesc = readChunk(data, map_size, &new_chunk, offset);
        if(bytesc == -1) {
            fprintf(stderr, "Error while reading chunk\n");
            return 1;
        }

        if(!new_chunk.valid) {
            fprintf(stderr, "Chunk CRC mismatch, corrupted chunk\n");
            fprintf(stderr, "Exiting\n");
            return 1;
        }

        chunk_buffer[chunks_size] = new_chunk;
        if(chunk_buffer[chunks_size].type == IEND) found_end = true;
        offset += CHUNK_SIZE(chunk_buffer[chunks_size]);
        chunks_size++;
    }

    if(cb_size) *cb_size = chunks_size;
    unmapFile(data, map_size);
    return 1;
}

int writeChunk(int fd, chunk src, size_t offset) {
    uint32_t length = src.length;
    ssize_t bytesw = offset;

    REVERSE(src.length);
    REVERSE(src.type);
    REVERSE(src.crc);

    if((bytesw += pwrite(fd, &src.length, sizeof(src.length), bytesw)) == -1) return -1;
    if((bytesw += pwrite(fd, &src.type, sizeof(src.type), bytesw)) == -1) return -1;
    if((bytesw += pwrite(fd, src.data, length, bytesw)) == -1) return -1;
    if((bytesw += pwrite(fd, &src.crc, sizeof(src.crc), bytesw)) == -1) return -1;

    return CHUNK_SIZE(src);
}

const char *getChunkName(uint32_t type) {
    switch(type) {
        case IHDR: return "IHDR";
        case PLTE: return "PLTE";
        case IDAT: return "IDAT";
        case IEND: return "IEND";
        case bKGD: return "bKGD";
        case cHRM: return "cHRM";
        case gAMA: return "gAMA";
        case hIST: return "hIST";
        case iCCP: return "iCCP";
        case iTXt: return "iTXt";
        case pHYs: return "pHYs";
        case sBIT: return "sBIT";
        case sPLT: return "sPLT";
        case sRGB: return "sRGB";
        case tEXt: return "tEXt";
        case tIME: return "tIME";
        case tRNS: return "tRNS";
        case zTXt: return "zTXt";
    }
    return "undefined";
}

void printChunk(chunk tchunk) {
    printf("------------\n");
    printf("Size: %u bytes\n", tchunk.length);
    printf("Type: (%s) 0X%X\n", getChunkName(tchunk.type), tchunk.type);

    printf("Data: %p\n", tchunk.data);
    printf("CRC: 0x%0X\n", tchunk.crc);
    printf("CRC check: %s\n", tchunk.valid ? "Pass" : "Corrupted");
    printf("------------\n");
}


int readChunk(void *data, size_t data_size, chunk *buff, size_t offset) {
    size_t start = offset;

    uint32_t length;
    memcpy(&length, (uint8_t *)data + offset, sizeof(length));
    REVERSE(length);
    buff->length = length;

    if((offset + sizeof(length)) >= data_size) return -1;
    offset += sizeof(length);

    uint32_t type;
    memcpy(&type, (uint8_t *)data + offset, sizeof(type));

    REVERSE(type);
    buff->type = type;

    if((offset + sizeof(type)) >= data_size) return -1;
    offset += sizeof(type);

    void *addr = NULL;
    addr = malloc(length);
    if(!addr) return -1;
    buff->data = addr;
    memcpy(buff->data, (char *)data + offset, length);

    if((offset + length) >= data_size) return -1;
    offset += length;

    uint32_t crc;
    memcpy(&crc, (char *)data + offset, sizeof(crc));
    REVERSE(crc);
    buff->crc = crc;

    uint32_t cyc = calcCRC((uint8_t *)data + start + sizeof(length), sizeof(type) + length);
    buff->valid = cyc == crc;
    return 1;
}

int freeChunk(chunk *target_chunk) {
    if(!target_chunk || !target_chunk->data) return -1;
    free(target_chunk->data);
    target_chunk->data = NULL;
    target_chunk->type = 0;
    target_chunk->length = 0;
    target_chunk->crc = 0;
    target_chunk->valid = false;
    return 1;
}

int unmapFile(void *addr, size_t map_size) {
    if(!addr) return -1;
    munmap(addr,  map_size);
    return 1;
}

void reverse(void *buff, size_t size) {
    uint8_t temp;

    for(size_t idx = 0; idx < size / 2; idx++) {
        temp = *(uint8_t *)((uint8_t *)buff + idx);
        *(uint8_t *)((uint8_t *)buff + idx) = *(uint8_t *)((uint8_t *)buff + size - idx - 1);
        *(uint8_t *)((uint8_t *)buff + size - idx - 1) = temp;
    }
}

void *mapFile(int fd, size_t *map_size) {
    struct stat sbuf;
    if(fstat(fd, &sbuf) == 1) return NULL;
    void *data = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if(data == MAP_FAILED) return NULL;

    if(map_size) *map_size = sbuf.st_size;
    return data;
}

/*
    CRC algorithm, slightly modified
    Taken from http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html
*/

/* Table of CRCs of all 8-bit messages. */
unsigned long crc_table[256];

/* Flag: has the table been computed? Initially false. */
int crc_table_computed = 0;

/* Make the table for a fast CRC. */
static void make_crc_table(void) {
  unsigned long c;
  int n, k;

  for(n = 0; n < 256; n++) {
    c = (unsigned long) n;
    for(k = 0; k < 8; k++) {
      if(c & 1)
        c = 0xedb88320L ^ (c >> 1);
      else
        c = c >> 1;
    }
    crc_table[n] = c;
  }
  crc_table_computed = 1;
}

/* Update a running CRC with the bytes buf[0..len-1]--the CRC
   should be initialized to all 1's, and the transmitted value
   is the 1's complement of the final running CRC (see the
   crc() routine below)). */

static unsigned long update_crc(unsigned long crc, unsigned char *buf, int len) {
  unsigned long c = crc;
  int n;

  if(!crc_table_computed)
    make_crc_table();
  for(n = 0; n < len; n++) {
    c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
  }
  return c;
}

/* Return the CRC of the bytes buf[0..len-1]. */
unsigned long calcCRC(unsigned char *buf, int len) {
  return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}
