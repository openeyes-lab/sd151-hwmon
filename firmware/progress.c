#include <stdio.h>
#include <string.h>
#include "progress.h"

/** \brief Print progress bar with prefix
 *
 * \param [in] iteration Current iteration
 * \param [in] total Total number of iterations
 * \param [in] prefix Prefix text
 * \param [in] fill Char to use for filling the bar
 * \return Noting
 *
 */
void PROGRESS_Print(uint16_t iteration,
                        uint16_t total, char *prefix, char fill, char *note)
{
  uint8_t filledLength;
  float percent;
  char bar[PROGRESS_BAR_LENGTH + 1];
  char bar2[PROGRESS_BAR_LENGTH + 1];

  memset(bar, fill, PROGRESS_BAR_LENGTH);
  bar[PROGRESS_BAR_LENGTH] = 0;
  memset(bar2, ' ', PROGRESS_BAR_LENGTH);
  bar2[PROGRESS_BAR_LENGTH] = 0;
  percent = (float)iteration / total * 100;
  filledLength = (uint8_t)(PROGRESS_BAR_LENGTH * iteration / total);

  printf("\r%s [%.*s%.*s] %.1f%% %s", prefix, filledLength, bar,
                  PROGRESS_BAR_LENGTH - filledLength, bar2, percent, note);
  fflush(stdout);

  // Print New Line on Complete
  if (iteration >= total)
    printf("\n");
}

/** \brief Do break in the output
 *
 * \return Nothing
 *
 */
void PROGRESS_Break(void)
{
  printf("\n");
}
