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

#define REVERSE(A) reverse(&(A), sizeof(A))

enum chunk_type {
    IHDR = 0x49484452,
    PLTE = 0x504c5445,
    IDAT = 0x49444154,
    IEND = 0x49454e44,


    // Ancillary chunks
    bKGD = 0x624b4744,
    cHRM = 0x6348524d,
    gAMA = 0x67414d41,
    hIST = 0x68495354,
    iCCP = 0x69434350,
    iTXt = 0x69545874,
    pHYs = 0x70485973,
    sBIT = 0x73424954,
    sPLT = 0x73504c54,
    sRGB = 0x73524742,
    tEXt = 0x74455874,
    tIME = 0x74494d45,
    tRNS = 0x74524e53,
    zTXt = 0x7a545874,
};

/* Table of CRCs of all 8-bit messages. */
unsigned long crc_table[256];

/* Flag: has the table been computed? Initially false. */
int crc_table_computed = 0;

/* Make the table for a fast CRC. */
void make_crc_table(void) {
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

unsigned long update_crc(unsigned long crc, unsigned char *buf, int len) {
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


void reverse(void *buff, size_t size) {
    uint8_t temp;

    for(size_t idx = 0; idx < size / 2; idx++) {
        temp = *(uint8_t *)(buff + idx);
        *(uint8_t *)(buff + idx) = *(uint8_t *)(buff + size - idx - 1);
        *(uint8_t *)(buff + size - idx - 1) = temp;
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

    close(fd);

    size_t offset = SIGNITURE_SIZE;

    uint32_t length;
    memcpy(&length, data + offset, sizeof(length));
    REVERSE(length);
    printf("%u\n", length);

    offset += sizeof(length);

    uint32_t type;
    memcpy(&type, data + offset, sizeof(type));

    REVERSE(type);
    printf("0x%X\n", type);

    if(type == IHDR) {
        printf("IHDR chunk\n");
    }

    offset += sizeof(type);
    offset += length;

    uint32_t crc;
    memcpy(&crc, data + offset, sizeof(crc));
    REVERSE(crc);

    uint32_t cyc = calcCRC(data + SIGNITURE_SIZE + sizeof(length), sizeof(type) + length);

    printf("0x%X\n", crc);
    printf("0x%X\n", cyc);

    munmap(data, map_size);
    return 0;
}
