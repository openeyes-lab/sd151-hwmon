#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "ihex.h"


/**
 * @brief Get one nibble from char
 * @param [in] c Char value
 * @return data nibble as uint8_t
 */
uint8_t IHEX_GetNibble(char c)
{
  if (c >= '0' && c <= '9')
    return (uint8_t)(c - '0');
  else if (c >= 'A' && c <= 'F')
    return (uint8_t)(c - 'A' + 10);
  else if (c >= 'a' && c <= 'f')
    return (uint8_t)(c - 'a' + 10);
  else
    return 0;
}

/**
 * @brief Get full byte from two chars
 * @param [in] data Two chars as HEX representation
 * @return byte value as uint8_t
 */
uint8_t IHEX_GetByte(char *data)
{
  uint8_t res = IHEX_GetNibble(*data++) << 4;
  res += IHEX_GetNibble(*data);
  return res;
}

/**
 * @brief parsing of HEX file
 * @param [in] par updiParam struct
 * @return 0 on success
 */
int ParseHEXFile(updiParam *par)
{
  uint16_t addr,i,j;
  uint16_t segment;
  uint8_t  type;
  uint8_t  len;
  uint8_t  byte;
  uint8_t  *ptd=NULL;
  uint8_t  *ptf=NULL;
  char     str[128];
  char     *end;
  FILE     *fp;
  int      err;

  addr = 0;
  segment = 0;
  par->flash_max_used = 0;

  /* Open source HEX file */
  if ((fp = fopen(par->filename, "rt")) == NULL)
    return -ENOENT;

  /* Allocate Flash memory */
  par->flash_data = malloc(par->flash_size);
  if (!par->flash_data)
  {
    fclose(fp);
    return -ENOMEM;
  }

  ptd = par->flash_data;
  memset(ptd,0xff,par->flash_size);

  /* Scan file */
  while (!feof(fp))
  {
    /* Get line */
    if (fgets(str, sizeof(str), fp) == NULL) {
      err = -EIO;
      goto err_close;
    }

    /* clean line from space and control chars */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char) *end)) end--;
    end[1] = '\0';

    /* Check line lenght */
    if (strlen(str) < IHEX_MIN_STRING) {
      err = -EBFONT;
      goto err_close;
    }

    len = IHEX_GetByte(&str[IHEX_OFFS_LEN]);
    addr = (IHEX_GetByte(&str[IHEX_OFFS_ADDR]) << 8) + IHEX_GetByte(&str[IHEX_OFFS_ADDR + 2]);
    type = IHEX_GetByte(&str[IHEX_OFFS_TYPE]);

    if (len * 2 + IHEX_MIN_STRING != strlen(str)) {
      err = -EBFONT;
      goto err_close;
    }

    switch (type)
    {
      case IHEX_DATA_RECORD:
        switch (segment) {
          case ATTINY_DATA_SEG:
            if (ptd==NULL) {
              err = -ENOMEM;
              goto err_close;
            }
            for (i = 0; i < len; i++)
            {
              j=addr+i;
              if (j>par->flash_size) {
                err = -EBFONT;
                goto err_close;
              }
              byte = IHEX_GetByte(&str[IHEX_OFFS_DATA + i * 2]);
              if ((byte != 0xFF) && (j>=par->flash_max_used))
                par->flash_max_used = j + 1;
              *(ptd+j) = byte;
            }
            break;
          case ATTINY_FUSE_SEG:
            if (ptf==NULL) {
              err = -ENOMEM;
              goto err_close;
            }
            for (i = 0; i < len; i++)
            {
              j=addr+i;
              if (j>par->number_of_fuses) {
                err = -EBFONT;
                goto err_close;
              }
              byte = IHEX_GetByte(&str[IHEX_OFFS_DATA + i * 2]);

              *(ptf+j) = byte;
            }
            break;
          default:
            err = -EBFONT;
            goto err_close;
        }
        break;
      case IHEX_END_OF_FILE_RECORD:
        return IHEX_ERROR_NONE;
      case IHEX_EXTENDED_SEGMENT_ADDRESS_RECORD:
      case IHEX_START_LINEAR_ADDRESS_RECORD:
      case IHEX_START_SEGMENT_ADDRESS_RECORD:
        /* NOT supported */
        err = -EBFONT;
        goto err_close;
        break;
      case IHEX_EXTENDED_LINEAR_ADDRESS_RECORD:
        segment = ((IHEX_GetByte(&str[IHEX_OFFS_DATA]) << 8) + IHEX_GetByte(&str[IHEX_OFFS_DATA + 2]));
        switch (segment) {
          case ATTINY_DATA_SEG:
            break;
          case ATTINY_FUSE_SEG:
            par->fuse_data = malloc(par->number_of_fuses);
            if (!par->fuse_data)
            {
              err = -ENOMEM;
              goto err_close;
            }
            ptf = par->fuse_data;
            break;
          default:
            err = -EBFONT;
            goto err_close;
        }
        break;
      default:
        err = -EBFONT;
        goto err_close;
    }
  }

  return 0;

err_close:
  fclose(fp);
  free(par->flash_data);
  free(par->fuse_data);
  return err;
}
