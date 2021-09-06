#include <string.h>
#include "phy.h"
#include "nvm.h"
#include "updi.h"

/**
 * @brief Load data from Control/Status space
 * @param [in] address
 * @return byte returned from UPDI
 */
uint8_t load_cs(uint8_t address)
{
  uint8_t response = 0;
  uint8_t buf[] = {UPDI_PHY_SYNC, UPDI_LDCS | (address & 0x0F)};

  PHY_Send(buf, sizeof(buf));
  PHY_Receive(&response, 1);
  return response;
}

/**
 * @brief Store a value to Control/Status space
 * @param [in] address
 * @param [in] value
 * @return None
 */
void store_cs(uint8_t address, uint8_t value)
{
  uint8_t buf[] = {UPDI_PHY_SYNC, UPDI_STCS | (address & 0x0F), value};

  PHY_Send(buf, sizeof(buf));
}

/**
 * @brief Check UPDI by loading CS STATUSA
 * @return true if link is up
 */
static bool CheckLink(void)
{
  if (load_cs(UPDI_CS_STATUSA) != 0)
    return true;
  return false;
}

/**
 * @brief Set the inter-byte delay bit and disable collision detection
 * @param None
 * @return None
 */
static void LinkStart(void)
{
  store_cs(UPDI_CS_CTRLB, 1 << UPDI_CTRLB_CCDETDIS_BIT);
  store_cs(UPDI_CS_CTRLA, 1 << UPDI_CTRLA_IBDLY_BIT);
}

/**
 * @brief Initialize link with device
 * @param None
 * @return 0 on success.
 */
int LinkInit(void)
{
  uint8_t err = 3;

  while (err-- > 0)
  {
    DoubleBreak();

    LinkStart();

    //Check answer
    if (CheckLink() == true) {
      if (EnterProgmode() == true)
        return 0;
    }
  }

  return -1;
}

/**
 * @brief Load a single byte direct from a 16-bit address
 * @param address
 * @return byte read.
 */
uint8_t LoadByte(uint16_t address)
{
  //Load a single byte direct from a 16-bit address
  uint8_t response;
  uint8_t buf[] = {UPDI_PHY_SYNC, UPDI_LDS | UPDI_ADDRESS_16 | UPDI_DATA_8, address & 0xFF, (address >> 8) & 0xFF};

  PHY_Send(buf, sizeof(buf));
  PHY_Receive(&response, 1);
  return response;
}

/**
 * @brief Store a single byte value directly to a 16-bit address
 * @param [in] address
 * @param [in] byte
 * @return True if success.
 */
bool StoreByte(uint16_t address, uint8_t value)
{
  uint8_t response;
  uint8_t buf[] = {UPDI_PHY_SYNC, UPDI_STS | UPDI_ADDRESS_16 | UPDI_DATA_8, address & 0xFF, (address >> 8) & 0xFF};

  // prepare for write
  PHY_Send(buf, sizeof(buf));
  PHY_Receive(&response, 1);
  if (response != UPDI_PHY_ACK)
    return false;

  // write byte
  PHY_Send(&value, 1);
  PHY_Receive(&response, 1);
  if (response != UPDI_PHY_ACK)
    return false;

  return true;
}

/**
 * @brief Loads a number of bytes from the pointer location with pointer
 * post-increment
 * @param [in] data buffer to write
 * @param [in] size
 * @return True if success.
 */
int LoadByte_ptr_inc(uint8_t *data, uint8_t size)
{
  uint8_t buf[] = {UPDI_PHY_SYNC, UPDI_LD | UPDI_PTR_INC | UPDI_DATA_8};

  PHY_Send(buf, sizeof(buf));

  return PHY_Receive(data, size);
}

/**
 * @brief Set the pointer location
 * @param [in] address
 * @return True if success.
 */
bool StoreByte_ptr(uint16_t address)
{
  //Set the pointer location
  uint8_t response;
  uint8_t buf[] = {UPDI_PHY_SYNC, UPDI_ST | UPDI_PTR_ADDRESS | UPDI_DATA_16, address & 0xFF, (address >> 8) & 0xFF};

  PHY_Send(buf, sizeof(buf));
  PHY_Receive(&response, 1);
  if (response != UPDI_PHY_ACK)
    return false;
  return true;
}

/** \brief
 *
 * \param
 * \param
 * \return
 *
 */
bool StoreByte_ptr_inc(uint8_t *data, uint16_t len)
{
  //Store data to the pointer location with pointer post-increment
  uint8_t response;
  uint16_t n;
  uint8_t buf[] = {UPDI_PHY_SYNC, UPDI_ST | UPDI_PTR_INC | UPDI_DATA_8, data[0]};

  PHY_Send(buf, sizeof(buf));
  PHY_Receive(&response, 1);
  if (response != UPDI_PHY_ACK)
    return false;

  n = 1;
  while (n < len)
  {
    PHY_Send(&data[n], sizeof(uint8_t));
    PHY_Receive(&response, 1);
    if (response != UPDI_PHY_ACK)
      return false;
    n++;
  }

  return true;
}


/** \brief
 *
 * \param
 * \param
 * \return
 *
 */
void LINK_Repeat(uint16_t repeats)
{
  //Store a value to the repeat counter
  uint8_t buf[] = {UPDI_PHY_SYNC, UPDI_REPEAT | UPDI_REPEAT_WORD, (repeats - 1) & 0xFF, ((repeats - 1) >> 8) & 0xFF};

  PHY_Send(buf, sizeof(buf));
}

/** \brief
 *
 * \param
 * \param
 * \return
 *
 */
bool SendKey(char *key, uint8_t size)
{
  //Write a key
  uint8_t buf[] = {UPDI_PHY_SYNC, UPDI_KEY | UPDI_KEY_KEY | size};
  uint8_t data[8];
  uint8_t n, i;

  if (strlen(key) != (8 << size))
    return false;

  PHY_Send(buf, sizeof(buf));
  n = strlen(key);
  i = 0;
  while (n > 0)
  {
    data[i++] = key[n - 1];
    n--;
  }
  PHY_Send(data, sizeof(data));

  return true;
}
