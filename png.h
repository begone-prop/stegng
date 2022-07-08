#ifndef PNG_H__
#define PNG_H__

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#define SIGNITURE_SIZE 8
static const uint8_t validSigniture[] = {137, 80, 78, 71, 13, 10, 26, 10};
#define HAS_VALID_SIGNITURE(A) \
    (memcmp((A), (validSigniture), SIGNITURE_SIZE) == 0 ? true : false)

#define REVERSE(A) reverse(&(A), sizeof(A))
#define CHUNK_SIZE(a) (a).length + 12

typedef struct chunk {
  uint32_t length;
  uint32_t type;
  void *data;
  uint32_t crc;
  bool valid;
} chunk;

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

int readChunk(void *, size_t, chunk *, size_t);
void *mapFile(int, size_t *);
unsigned long calcCRC(unsigned char *, int);
unsigned long update_crc(unsigned long, unsigned char *, int);
void make_crc_table(void);
#endif