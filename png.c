#include "./png.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>

static void reverse(void *, size_t);

int readChunk(void *data, size_t data_size, chunk *buff, size_t offset) {
    size_t start = offset;

    uint32_t length;
    memcpy(&length, data + offset, sizeof(length));
    REVERSE(length);
    buff->length = length;

    if((offset + sizeof(length)) >= data_size) return -1;
    offset += sizeof(length);

    uint32_t type;
    memcpy(&type, data + offset, sizeof(type));

    REVERSE(type);
    buff->type = type;

    if((offset + sizeof(type)) >= data_size) return -1;
    offset += sizeof(type);

    void *addr = NULL;
    addr = malloc(length);
    if(!addr) return -1;
    buff->data = addr;
    memcpy(buff->data, data + offset, length);

    if((offset + length) >= data_size) return -1;
    offset += length;

    uint32_t crc;
    memcpy(&crc, data + offset, sizeof(crc));
    REVERSE(crc);
    buff->crc = crc;

    uint32_t cyc = calcCRC(data + start + sizeof(length), sizeof(type) + length);
    buff->valid = cyc == crc;
    return 1;
}

static void reverse(void *buff, size_t size) {
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

/*
    CRC algorithm, slightly modified
    Taken from http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html
*/

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