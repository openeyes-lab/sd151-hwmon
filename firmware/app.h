#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdbool.h>
#include "updi.h"

bool ForceProgmode(void);
void ExitProgmode(void);
bool APP_WaitFlashReady(updiParam *);
bool APP_Unlock(void);
int APP_ChipErase(updiParam *);
bool APP_ReadDataWords(uint16_t address, uint8_t *data, uint16_t words);
//bool APP_WriteData(uint16_t address, uint8_t *data, uint16_t len);
int PageWrite(updiParam *, uint16_t, uint8_t *);
int PageRead(updiParam *, uint16_t, uint8_t *);
bool APP_ReadData(uint16_t address, uint8_t *data, uint16_t size);
int PageErase(updiParam *, uint16_t);

#endif
