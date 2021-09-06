#ifndef NVM_H
#define NVM_H

#include <stdint.h>
#include <stdbool.h>

#define NVM_MAX_ERRORS            (3)
#define ERASE_MAX_LINK_ERROR      (3)
#define ERASE_MAX_BYTE_ERROR      (10)
#define VERIFY_MAX_LINK_ERROR      (3)
#define VERIFY_MAX_DATA_ERROR      (10)

bool EnterProgmode(void);
void LeaveProgmode(void);
bool NVM_ChipErase(updiParam *);
uint8_t NVM_ReadFuse(uint8_t fusenum);
bool NVM_WriteFuse(uint8_t fusenum, uint8_t value);
bool NVM_LoadIhex(updiParam *data);
bool NVM_SaveIhex(char *filename, uint16_t address, uint16_t len);
int FlashBlankCheck(updiParam *);
int ChipErase(updiParam *);
int FlashProgram(updiParam *);
int FlashVerify(updiParam *);

#endif
