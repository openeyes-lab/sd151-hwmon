/**
 * @mainpage UPDI single wire programmer
 *
 * @section intro_sec Introduction
 *
 * This program allow the upgrade of new firmware to the ATTiny817 MCU.
 * The firmware is downloaded throght a single wire connection using the
 * raspberry GPIO24.
 * The main problem remain the fact that the trasmission media is a single wire
 * and serialization is made by software, this mean that the speed is very slow
 * to avoid problems in jitter on data.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include "updi.h"
#include "link.h"
#include "nvm.h"
#include "phy.h"
#include "ihex.h"
#include "app.h"

#define ENABLE_VERIFY
#define ENABLE_ERASE
#define ENABLE_WRITE

/**
 * @brief Main application function
 * @param [in] argc Number of command line arguments
 * @param [in] argv Command line arguments
 * @return exit code for OS
 */
int main(int argc, char* argv[])
{
  updiParam priv;
  int       err=0;
  uid_t     uid=getuid();

  if (uid!=0) {
    printf("\n\nThis program must be run with sudo\n\n");
    return 0;
  }

  memset(&priv, 0, sizeof(updiParam));

  priv.updi_pin = 5;
  priv.baudrate = 1000;
  /* ATTiny817 setting */
  priv.flash_start = 0x8000;
  priv.flash_size = 8 * 1024;
  priv.flash_pagesize = 64;
  priv.syscfg_address = 0x0F00;
  priv.nvmctrl_address = 0x1000;
  priv.sigrow_address = 0x1100;
  priv.fuses_address = 0x1280;
  priv.number_of_fuses = 9;
  priv.userrow_address = 0x1300;

  strcpy(priv.filename, "sd151.hex");

  //Create a UPDI physical connection
  err=PHY_Init(&priv);
  if (err) {
    printf("ERROR! Cannot initialize PHY err=%x",err);
    return err;
  }

  printf ("Updi programmer parsing file: '%s'\n",priv.filename) ;
  err = ParseHEXFile(&priv);
  if (err) {
    printf("ERROR! Cannot parse file err=%d\n",err);
    return err;
  }
  printf("done! Loaded %d bytes\n",priv.flash_max_used);

#ifdef ENABLE_ERASE
  printf("Wait erase .... be patient\n");
  if (ChipErase(&priv)) {
    printf("ERROR! Cannot erase Flash.\n");
    return -1;
  }

  if (FlashBlankCheck(&priv)) {
    printf("Flash NOT erased\n");
    return -1;
  }
  printf("Flash erased\n");
#endif

#ifdef ENABLE_WRITE
  err = FlashProgram(&priv);
  if (err) {
    printf("Cannot program Flash err=%d\n",err);
    return -1;
  }
  printf("Writing terminated OK\n");
#endif

#ifdef ENABLE_VERIFY
  err = FlashVerify(&priv);
  if (err) {
    printf("Verify failed!\n");
    return -1;
  }
  printf("Verifing terminated OK\n");
#endif

  LeaveProgmode();

  PHY_Close();

  return 0;
}
