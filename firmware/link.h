#ifndef LINK_H
#define LINK_H

#include <stdint.h>
#include <stdbool.h>
#include "updi.h"

uint8_t load_cs(uint8_t address);
void store_cs(uint8_t address, uint8_t value);
bool SendKey(char *key, uint8_t size);
uint8_t LoadByte(uint16_t address);
bool StoreByte(uint16_t address, uint8_t value);
bool StoreByte16(uint16_t address, uint16_t value);
void LINK_Repeat(uint16_t repeats);
int LoadByte_ptr_inc(uint8_t *data, uint8_t size);
int LoadByte_ptr_inc16(uint8_t *data, uint16_t words);
bool StoreByte_ptr(uint16_t address);
bool StoreByte_ptr_inc(uint8_t *data, uint16_t len);
bool StoreByte_ptr_inc16(uint8_t *data, uint16_t len);
int LinkInit(void);

#endif
