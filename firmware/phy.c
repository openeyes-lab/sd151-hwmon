#include <unistd.h>
#include <errno.h>
#include <wiringPi.h>
#include "phy.h"
#include "updi.h"

static int loc_updi_pin=-1;
#ifdef DEBUG_PIN
static int loc_debug_pin=-1;
#endif

#define BIT_LEN               1000
#define RX_START_BIT_TIMEOUT  5000

/**
 * @brief Initialize physical interface
 *
 * @param [in] port Port name as string
 * @return 0 if success
 *
 */
int PHY_Init(updiParam *updi)
{
  if (wiringPiSetup () == -1)
    return -EIO ;

  pinMode (updi->updi_pin, INPUT) ;         // aka BCM_GPIO pin 17
  digitalWrite (updi->updi_pin, 1) ;       // End reset
  loc_updi_pin = updi->updi_pin;

#ifdef DEBUG_PIN
  pinMode (updi->debug_pin, OUTPUT) ;         // aka BCM_GPIO pin 17
  digitalWrite (updi->debug_pin, 0) ;
  loc_debug_pin = updi->debug_pin;
#endif

  piHiPri(90);

  return 0;
}

/**
 * @brief Sends a double break to reset the UPDI port
 * @param none
 * @return always 0 (success)
 * @details BREAK is actually just a slower zero frame. A double break is
 * guaranteed to push the UPDI state machine into a known state,
 * albeit rather brutally
 *
 */
int DoubleBreak(void)
{
  if (loc_updi_pin == -1)
    return -EIO;

  /* Set updi pin as output and force it to 0 */
  pinMode (loc_updi_pin, OUTPUT) ;
  digitalWrite (loc_updi_pin, 0) ;       // Start reset
  usleep(40000);  // wait for 40ms second
  digitalWrite (loc_updi_pin, 1) ;       // End reset
  usleep(50000);  // wait for 50ms second
  digitalWrite (loc_updi_pin, 0) ;       // Start reset
  usleep(40000);  // wait for 40ms second
  digitalWrite (loc_updi_pin, 1) ;       // End reset
  usleep(10000);  // wait for 10ms second

  return 0;
}

/** \brief Send data to physical interface
 *
 * \param [in] data Buffer with data
 * \param [in] len Length of data buffer
 * \return true if success
 *
 */
int PHY_Send(uint8_t *data, uint8_t len)
{
  uint8_t  i,bit;
  uint8_t  *pt=data;
  uint8_t  byte2send;
  uint16_t bitlen = BIT_LEN;
  uint8_t  parity = 0;

  if (loc_updi_pin == -1)
    return -1;

  pinMode (loc_updi_pin, OUTPUT) ;         // aka BCM_GPIO pin 17
  digitalWrite (loc_updi_pin, 1) ;       // End reset
  usleep(10*bitlen);

  for (i = 0; i < len; i++)
  {
    byte2send = *pt++;

    /* send start bit */
    digitalWrite (loc_updi_pin, 0) ;
    usleep(bitlen);
    parity=0;
    /* send byte */
    for (bit=0;bit<8;bit++) {
      digitalWrite (loc_updi_pin, (byte2send&1)) ;
      usleep(bitlen);
      if (byte2send&1)
        parity++;
      byte2send = byte2send>>1;
    }
    /* Send parity */
    digitalWrite (loc_updi_pin, (parity&1)) ;
    usleep(bitlen);
    /* Send 2 stop bit */
    digitalWrite (loc_updi_pin, 1) ;
    usleep(8*bitlen);

  }

  return 0;
}

/**
 * @brief Receive data from physical interface to data buffer
 *
 * @param [out] data Data buffer to write data in
 * @param [in] len Length of data to be received
 * @return num of rx bytes.
 *
 * @details
 * TODO parity and stop bits check
 *
 */
int PHY_Receive(uint8_t *data, uint16_t len)
{
  uint8_t  *pt=data;
  int      timer;
  int      rxlen = 0;
  int      bit;
  uint8_t  rxbit,byte;

#ifdef DEBUG_PIN
  digitalWrite (loc_debug_pin, 1) ;
#endif

  pinMode (loc_updi_pin, INPUT) ;         // aka BCM_GPIO pin 17

  for (rxlen=0;rxlen<len;rxlen++) {
    timer = 0;
    /* wait start bit */
    while (digitalRead(loc_updi_pin)) {
        usleep(BIT_LEN/8);
        if(++timer>RX_START_BIT_TIMEOUT)
          return rxlen;
    }
    /* start bit detected */
#ifdef DEBUG_PIN
    digitalWrite (loc_debug_pin, 0) ;
#endif
    /* wait some time to position the sample point */
    usleep(BIT_LEN/8);

    /* load byte */
    byte = 0;
    for (bit=0;bit<8;bit++) {
      usleep(BIT_LEN);
#ifdef DEBUG_PIN
      digitalWrite (loc_debug_pin, 1) ;
#endif
      rxbit = (digitalRead(loc_updi_pin))?0x80:0x00;
      byte = (byte>>1) | rxbit;
#ifdef DEBUG_PIN
      digitalWrite (loc_debug_pin, 0) ;
#endif
    }
    /* save received byte */
    *pt = byte & 0xff;
    pt++;

    /* wait stop bit and parity */
    usleep(3*BIT_LEN);
#ifdef DEBUG_PIN
    digitalWrite (loc_debug_pin, 1) ;
#endif
}
#ifdef DEBUG_PIN
  digitalWrite (loc_debug_pin, 0) ;
#endif

  return rxlen;
}

/** \brief Close physical interface
 *
 * \return Nothing
 *
 */
void PHY_Close(void)
{
  /* release tx pin */
  if (loc_updi_pin != -1) {
    pinMode (loc_updi_pin, INPUT) ;
    loc_updi_pin = -1;
  }
#ifdef DEBUG_PIN
  /* release rx pin */
  if (loc_debug_pin != -1) {
    pinMode (loc_debug_pin, INPUT) ;
    loc_debug_pin = -1;
  }
#endif
}
