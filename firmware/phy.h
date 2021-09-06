#ifndef PHY_H
#define PHY_H

#include <stdint.h>
#include <stdbool.h>
#include "updi.h"

#define PHY_BAUDRATE      (115200)

int PHY_Init(updiParam *);
int DoubleBreak(void);
int PHY_Send(uint8_t *data, uint8_t len);
int PHY_Receive(uint8_t *data, uint16_t len);
void PHY_Close(void);

#endif
