#ifndef IHEX_H
#define IHEX_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "updi.h"

#define IHEX_LINE_LENGTH    16
#define IHEX_MIN_STRING     11

#define IHEX_OFFS_LEN       1
#define IHEX_OFFS_ADDR      3
#define IHEX_OFFS_TYPE      7
#define IHEX_OFFS_DATA      9

#define IHEX_START          ":"
#define IHEX_NEWLINE        "\n"
#define IHEX_ENDFILE        ":00000001FF"

#define ATTINY_DATA_SEG     0x0000
#define ATTINY_FUSE_SEG     0x0082

enum {
  IHEX_DATA_RECORD,
  IHEX_END_OF_FILE_RECORD,
  IHEX_EXTENDED_SEGMENT_ADDRESS_RECORD,
  IHEX_START_SEGMENT_ADDRESS_RECORD,
  IHEX_EXTENDED_LINEAR_ADDRESS_RECORD,
  IHEX_START_LINEAR_ADDRESS_RECORD
};

enum {
  IHEX_ERROR_NONE,
  IHEX_ERROR_FILE,
  IHEX_ERROR_SIZE,
  IHEX_ERROR_FMT,
  IHEX_ERROR_CRC
};

#define IHEX_DIGIT(n) ((char)((n) + (((n) < 10) ? '0' : ('A' - 10))))

int ParseHEXFile(updiParam *);
void DumpFlashMemory(updiParam *);

#endif
