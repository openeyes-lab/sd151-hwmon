/**
 * @page NVM_MODULE Handle access to non volatile memory and fuse
 * @brief NVM control module
 *
 * @section nvme_description_sec Description
 * This module supports the first applicative layer to the user interface.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "app.h"
#include "link.h"
#include "nvm.h"
#include "progress.h"
#include "updi.h"

static bool moduleProgmode = false;

/**
 * @brief Enter programming mode
 * @param None
 * @return True if succeed
 */
bool EnterProgmode(void)
{
  moduleProgmode = ForceProgmode();
  return moduleProgmode;
}

/**
 * @brief Leave programming mode
 * @param None
 * @return Nothing
 */
void LeaveProgmode(void)
{
  ExitProgmode();
  moduleProgmode = false;
}

/**
 * @brief Page blanck check
 * @param [in] data pointer to data page
 * @param [in] size page len
 * @return 0 if all bytes are at 0xff -1  otherwise.
 * @details
 * Function that chack that all bytes into a page are erased (0xff)
 */
static int PageblanckCheck(uint8_t *data, uint16_t size)
{
  int i;
  uint8_t *pt=data;

  // check bytes
  for(i=0;i<size;i++) {
    if (*pt++!=0xff) {
      return -1;
    }
  }

  return 0;
}

/**
 * @brief Chip erase
 * @param [in] par updiParam
 * @return APP_ChipErase result
 * @details
 * Function wrapper calling APP_ChipErase
 */
int ChipErase(updiParam *par)
{
  if (moduleProgmode == false) {
    /* Reset MCU and initialize link */
    LinkInit();
    usleep(2000);
  }

  return APP_ChipErase(par);
}

/**
 * @brief Page verify
 * @param [in] tgt pointer to target page
 * @param [in] mem pointer to memory page
 * @param [in] size of page
 * @return 0 if equal -1 otherwise
 * @details
 * Function that check if page are equal.
 */
static int PageVerify(uint8_t *tgt,uint8_t *mem, uint16_t size)
{
  int i;
  uint8_t *pta=tgt;
  uint8_t *ptb=mem;

  // check bytes
  for(i=0;i<size;i++) {
    if (*pta++!=*ptb++) {
      //printf(">>>>>>>>Mismatch value at index=%d\n",i);
      //printf("Flash:\n");
      //for(j=0,pta=tgt;j<size;j++)
      //  printf("[%02x]",*pta++);
      //printf("\nMem:\n");
      //for(j=0,ptb=mem;j<size;j++)
      //  printf("[%02x]",*ptb++);
      //printf("\n\n");
      return 1;
    }
  }

  return 0;
}


/**
 * @brief Flash blanck check
 * @param [in] par updiParam
 */
int FlashBlankCheck(updiParam *par)
{
  uint16_t i;
  uint16_t pages;
  uint16_t address = par->flash_start;
  uint8_t page_size = par->flash_pagesize;
  uint8_t err_link = 0;
  uint8_t err_data = 0;
  uint8_t data[page_size];

  if (moduleProgmode == false) {
    /* Reset MCU and initialize link */
    LinkInit();
    usleep(2000);
  }

  // Find the number of pages
  pages = par->flash_size / page_size;
  if (par->flash_size % page_size != 0)
    pages++;

  i = 0;

  PROGRESS_Print(0, pages, "Blanck checking: ", '#', "");

  // Read out page-wise for convenience
  while (i < pages)
  {
    // First load page into data
    if (PageRead(par,address,data))
    {
      // error occurred, try once more
      if (++err_link > ERASE_MAX_LINK_ERROR)
      {
        printf("Unable to read page %d of %d\n",i,pages);
        PROGRESS_Break();
        return -1;
      }
      /* Reset MCU and initialize link */
      LinkInit();
      usleep(5000);  // wait for 5ms
      continue;
    }

    // then check all check bytes are erased (0xff)
    if (PageblanckCheck(data,page_size))
    {
      // error occurred, try once more
      if (++err_data > ERASE_MAX_BYTE_ERROR)
      {
        printf("byte not erased at page %d of %d\n",i,pages);
        PROGRESS_Break();
        return -1;
      }
      usleep(5000);  // wait for 5ms
      continue;
    }

    i++;
    address += page_size;
    err_data = 0;
    err_link = 0;
    PROGRESS_Print(i, pages, "Blanck checking: ", '#', "");
  }

  return 0;
}

/**
 * @brief Flash program
 * @param [in] par updiParam
 * @return 0 On success -1 otherwise
 * @details
 * Program and verify every sector. In case a sector is still erased a write is
 * made again. In case of bad write a page erase and rewrite is made.
 */
int FlashProgram(updiParam *par)
{
  uint8_t *pdata;
  uint8_t page_size = par->flash_pagesize;
  uint16_t pages;
  uint16_t i;
  uint8_t err_write = 0;
  uint8_t err_link = 0;
  uint8_t err_data = 0;
  uint8_t err_erase = 0;
  uint16_t address = par->flash_start;
  uint8_t rdata[page_size];
  char note[128];

  if (moduleProgmode == false) {
    /* Reset MCU and initialize link */
    LinkInit();
    usleep(2000);
  }

  // Find the number of pages
  pages = par->flash_max_used / page_size;
  if (par->flash_max_used % page_size != 0)
    pages++;

  i = 0;

#if USE_PROGRESS==1
  sprintf(note,"Writing %d bytes on %d pages\n",par->flash_max_used,pages);
  PROGRESS_Print(0, pages, "Writing: ", '#', note);
#else
  printf("Writing %d bytes on %d pages\n",par->flash_max_used,pages);
#endif

  // Program each page
  while (i < pages)
  {
#if USE_PROGRESS==0
    printf("Writing %d bytes on %d pages\n",page_size,i);
#endif
    pdata = par->flash_data + (i*page_size);
    if (!err_link) {
      if (PageWrite(par, address, pdata)) {
        err_write++;
#if USE_PROGRESS==1
        sprintf(note,"Error %d on writing page: %d\n",err_write,i);
        PROGRESS_Print(i, pages, "Writing: ", '#', "");
#else
        printf("Error writing page: %d\n",i);
#endif
        if (err_write > 3)
        {
#if USE_PROGRESS==1
          PROGRESS_Break();
#else
          printf("Error writing page: %d",i);
#endif
          return -1;
        }
        /* Reset MCU and initialize link */
        LinkInit();
        usleep(5000);  // wait for 5ms
        continue;
      }
    }

#if USE_PROGRESS==0
  printf("reading page\n");
#endif

    if (PageRead(par,address,rdata))
    {
      err_link++;
#if USE_PROGRESS==1
      sprintf(note,"Error %d on reading page: %d\n",err_link,i);
      PROGRESS_Print(i, pages, "Writing: ", '#', "");
#else
      printf("Error reading page: %d\n",i);
#endif
      // error occurred, try once more
      if (err_link > 3)
      {
#if USE_PROGRESS==1
        PROGRESS_Break();
#else
        printf("Error reading page: %d",i);
#endif
        return -2;
      }
      /* Reset MCU and initialize link */
      LinkInit();
      usleep(5000);  // wait for 5ms
      continue;
    }

#if USE_PROGRESS==0
  printf("Verifing page\n");
#endif

    if (PageVerify(rdata,pdata,page_size))
    {
#if USE_PROGRESS==1
      sprintf(note,"Error %d on verifing page: %d\n",err_link,i);
      PROGRESS_Print(i, pages, "Writing: ", '#', "");
#else
      printf("Error verifing page: %d\n",i);
#endif
      if (PageblanckCheck(rdata,page_size)) {
        if (++err_link > 3)
        {
          if (++err_erase > 3)
          {
            PROGRESS_Break();
            return -3;
          }
          printf("\nErase page %d\n",i);
          if (PageErase(par,address)) {
            PROGRESS_Break();
            return -4;
          }
          err_link = 0;
          LinkInit();
          usleep(5000);  // wait for 5ms second
          continue;
        }
      } else {
        err_link = 0;
        if (++err_data > 3)
        {
          PROGRESS_Break();
          return -5;
        }
      }
      /* Reset MCU and initialize link */
      LinkInit();
      usleep(5000);  // wait for 5ms second
      continue;
    }

    err_link = 0;
    err_data = 0;
    err_erase = 0;
    err_write = 0;
    i++;
    // show progress bar
    PROGRESS_Print(i, pages, "Writing: ", '#', "");
    address += page_size;
  }

  return 0;
}


/**
 * @brief Flash Verify
 *
 */
int FlashVerify(updiParam *par)
{
  uint8_t *pdata;
  uint16_t i;
  uint16_t pages;
  uint16_t address = par->flash_start;
  uint8_t page_size = par->flash_pagesize;
  uint8_t err_link = 0;
  uint8_t err_data = 0;
  uint8_t data[page_size];

  if (moduleProgmode == false) {
    /* Reset MCU and initialize link */
    LinkInit();
    usleep(2000);
  }

  // Find the number of pages
  pages = par->flash_size / page_size;
  if (par->flash_size % page_size != 0)
    pages++;

  i = 0;

  PROGRESS_Print(0, pages, "Verify: ", '#', "");

  // Read out page-wise for convenience
  while (i < pages)
  {
  //  printf("verify page %d/%d\n",i,pages);

    if (PageRead(par,address,data))
    {
      // error occurred, try once more
      if (++err_link > VERIFY_MAX_LINK_ERROR)
      {
        printf("Error reading page %d/%d\n",i,pages);
        PROGRESS_Break();
        return -1;
      }
      /* Reset MCU and initialize link */
      LinkInit();
      usleep(5000);  // wait for 5ms before reply
      continue;
    }

    // check bytes
    pdata = par->flash_data + (i*page_size);
    if (PageVerify(data,pdata,page_size))
    {
      if (++err_data > VERIFY_MAX_DATA_ERROR)
      {
        PROGRESS_Break();
        return -1;
      }
      usleep(5000);  // wait for 5ms before reply
      continue;
    }

    i++;
    address += page_size;
    err_data = 0;
    err_link = 0;
    PROGRESS_Print(i, pages, "Verify: ", '#', "");

  }

  return 0;
}
