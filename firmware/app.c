#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "link.h"
#include "updi.h"


/**
 * @brief Applies or releases an UPDI reset condition
 * @param [in] reset status
 * @return None
 */
static void AssertReset(bool reset)
{
  if (reset == true)
    store_cs(UPDI_ASI_RESET_REQ, UPDI_RESET_REQ_VALUE);
  else
    store_cs(UPDI_ASI_RESET_REQ, 0x00);
}

/**
 * @brief Checks whether the NVM PROG flag is up
 * @param None
 * @return True if flag is up.
 */
static bool IsProgMode(void)
{
  if (load_cs(UPDI_ASI_SYS_STATUS) & (1 << UPDI_ASI_SYS_STATUS_NVMPROG))
    return true;
  return false;
}

/**
 * @brief Waits for the device to be unlocked.
 * @param [in] timeout_ms timeout in ms
 * @return True if flag is up.
 * @details All devices boot up as locked until proven otherwise
 */
static bool WaitUnlocked(uint16_t timeout_ms)
{
  while (timeout_ms-- > 0)
  {
    usleep(1000);
    if (!(load_cs(UPDI_ASI_SYS_STATUS) &
                (1 << UPDI_ASI_SYS_STATUS_LOCKSTATUS)))
      return true;
  }

  return false;
}

/**
 * @brief Put NVM in program mode.
 * @param [in] None
 * @return 0 if success.
 */
bool ForceProgmode(void)
{
  uint8_t key_status;

  //First check if NVM is already enabled
  if (IsProgMode() == true)
    return true;

  // Put in the key
  SendKey(UPDI_KEY_NVM, UPDI_KEY_64);

  // Check key status
  key_status = load_cs(UPDI_ASI_KEY_STATUS);

  if (!(key_status & (1 << UPDI_ASI_KEY_STATUS_NVMPROG)))
    return false;

  // Toggle reset
  AssertReset(true);
  AssertReset(false);

  // And wait for unlock
  if (!WaitUnlocked(100))
    return false;

  // Check for NVMPROG flag
  if (IsProgMode() == false)
    return false;

  return true;
}

/**
 * @brief Exit NVM from program mode.
 * @param None
 * @return None
 * @details Disables UPDI which releases any keys enabled
 */
void ExitProgmode(void)
{
  AssertReset(true);
  AssertReset(false);
  store_cs(UPDI_CS_CTRLB, (1 << UPDI_CTRLB_UPDIDIS_BIT) | (1 << UPDI_CTRLB_CCDETDIS_BIT));
}

/**
 * @brief Waits for the NVM controller to be ready
 * @param [in] par updiParam
 * @return True if Flash is ready
 */
static bool WaitFlashReady(updiParam *par)
{
  uint8_t status;
  uint16_t timeout = 10000; // 10 sec timeout, just to be sure

  while (timeout-- > 0)
  {
    usleep(1000);
    status = LoadByte(par->nvmctrl_address + UPDI_NVMCTRL_STATUS);
    if (status & (1 << UPDI_NVM_STATUS_WRITE_ERROR))
      return false;

    if (!(status & ((1 << UPDI_NVM_STATUS_EEPROM_BUSY) |
                                      (1 << UPDI_NVM_STATUS_FLASH_BUSY))))
      return true;
  }

  return false;
}

/**
 * @brief Executes an NVM COMMAND on the NVM CTRL
 * @param [in] par updiParam
 * @param [in] command to be executed
 * @return True if Flash is ready
 */
static bool ExecCommand(updiParam *par, uint8_t command)
{
  return StoreByte(par->nvmctrl_address + UPDI_NVMCTRL_CTRLA, command);
}

/**
 * @brief Chip erase using the NVM controller
 * @param [in] par updiParam
 * @return 0 on success.
 * @details Note that on locked devices this it not possible and the
 * ERASE KEY has to be used instead
 */
int APP_ChipErase(updiParam *par)
{
  // Wait until NVM CTRL is ready to erase
  if (!WaitFlashReady(par))
    return -1;

  // Erase
  ExecCommand(par,UPDI_NVMCTRL_CTRLA_CHIP_ERASE);

  // And wait for it
  if (!WaitFlashReady(par))
    return -1;

  return 0;
}

/**
 * @brief Writes bytes to memory
 * @param [in] address Flash address
 * @param [in] data incoming buffer
 * @param [in] len buffer size
 * @return 0 on success.
 */
static bool WriteData(uint16_t address, uint8_t *data, uint16_t len)
{
  // Range check
  if (len > (UPDI_MAX_REPEAT_SIZE + 1))
    return false;

  // Store the address
  StoreByte_ptr(address);

  // Fire up the repeat
  LINK_Repeat(len);
  return StoreByte_ptr_inc(data, len);
}

/**
 * @brief Read bytes from memory
 * @param [in] address Flash address
 * @param [out] data outgoing buffer
 * @param [in] len buffer size
 * @return 0 on success.
 */
static bool ReadData(uint16_t address, uint8_t *data, uint16_t size)
{
  // Range check
  if ((size > UPDI_MAX_REPEAT_SIZE + 1) || (size < 2))
    return false;

  // Store the address
  StoreByte_ptr(address);

  // Fire up the repeat
  LINK_Repeat(size);

  // Do the read(s)
  return LoadByte_ptr_inc(data, size);
}

/**
 * @brief Writes a page of data to NVM
 * @param [in] par updiParam struct
 * @param [in] address Flash address
 * @param [in] data incoming buffer
 * @return 0 on success.
 * @details
 * By default the PAGE_WRITE command is used, which
 * requires that the page is already erased.
 * By default word access is used (flash)
 */
int PageWrite(updiParam *par, uint16_t address, uint8_t *data)
{
  uint16_t len = par->flash_pagesize;
  // Check that NVM controller is ready
  if (!WaitFlashReady(par))
    return -1;

  // Clear the page buffer
  ExecCommand(par,UPDI_NVMCTRL_CTRLA_PAGE_BUFFER_CLR);

  // Waif for NVM controller to be ready
  if (!WaitFlashReady(par))
    return -1;

  // Load the page buffer by writing directly to location
  WriteData(address, data, len);

  // Write the page to NVM, maybe erase first
  ExecCommand(par,UPDI_NVMCTRL_CTRLA_WRITE_PAGE);

  // Wait for NVM controller to be ready again
  if (!WaitFlashReady(par))
    return -1;

  return 0;
}

/**
 * @brief Read page from memory
 * @param [in] address Flash address
 * @param [out] data buffer
 * @return 0 on success.
 */
int PageRead(updiParam *par, uint16_t address, uint8_t *data)
{
  uint16_t len = par->flash_pagesize;

  if (ReadData(address,data,len))
    return 0;

  return -1;
}

/**
 * @brief Earse a page of NVM
 * @param [in] par updiParam struct
 * @param [in] address Flash address
 * @return 0 on success.
 */
int PageErase(updiParam *par, uint16_t address)
{
  uint8_t data[64];

  memset(data,0xff,64);

  // Check that NVM controller is ready
  if (!WaitFlashReady(par))
    return -1;

  //SetErasePage(address);
  WriteData(address, data, 64);

  // Write the page to NVM, maybe erase first
  ExecCommand(par,UPDI_NVMCTRL_CTRLA_ERASE_PAGE);

  // Wait for NVM controller to be ready again
  if (!WaitFlashReady(par))
    return -1;

  return 0;
}
