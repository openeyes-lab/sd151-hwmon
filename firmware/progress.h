#ifndef PROGRESS_H
#define PROGRESS_H

#include <stdint.h>
#include <stdbool.h>

#define PROGRESS_BAR_LENGTH   (80)

void PROGRESS_Print(uint16_t iteration, uint16_t total, char *prefix, char fill, char *);
void PROGRESS_Break(void);

#endif // PROGRESS_H
