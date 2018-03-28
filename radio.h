 /*
 * -*- coding: UTF8 -*-
 * Semtech SX127x famaly chips example layer for SBC (Single Board Computer)
 * Tested on "Orange Pi Zero"
 * File: "radio.h"
 */

#ifndef RADIO_H
#define RADIO_H
//-----------------------------------------------------------------------------
#include "sx127x.h" // `sx127x_t`
//-----------------------------------------------------------------------------
#define ORANGE_PI_ZERO
//#define ORANGE_PI_ONE
//#define ORANGE_PI_WIN_PLUS

// GPIO lines and SPI devic to SX127x module
#if defined(ORANGE_PI_ZERO)
#  define RADIO_SPI_DEVICE  "/dev/spidev1.0" // on linux ver. 3.4.113-sun8i
#  define RADIO_GPIO_IRQ     6  // pin  7 of 26 (GPIO.7)
//#  define RADIO_GPIO_IRQ   1  // pin 11 of 26 (RxD2)
#  define RADIO_GPIO_RESET   7  // pin 12 of 26 (GPIO.1)
#  define RADIO_GPIO_CS      13 // pin 24 of 26 (CE0)
#  define RADIO_GPIO_DATA    19 // pin 16 of 26 (GPIO.4)
#  define RADIO_GPIO_LED     18 // pin 18 of 26 (GPIO.5)
//#  define RADIO_GPIO_LED   3  // pin 15 of 26 (CTS2)
//#  define RADIO_GPIO_LED   2  // pin 22 of 26 (RTS2)
#elif defined(ORANGE_PI_ONE)
#  define RADIO_SPI_DEVICE "/dev/spidev1.0" // FIXME
#  define RADIO_GPIO_IRQ   1 // FIXME
#  define RADIO_GPIO_RESET 2 // FIXME
#  define RADIO_GPIO_CS    3 // FIXME
#  define RADIO_GPIO_DATA  4 // FIXME
#  define RADIO_GPIO_LED   5 // FIXME
#elif defined(ORANGE_PI_WIN_PLUS)
#  define RADIO_SPI_DEVICE "/dev/spidev1.0" // FIXME
#  define RADIO_GPIO_IRQ   1 // FIXME
#  define RADIO_GPIO_RESET 2 // FIXME
#  define RADIO_GPIO_CS    3 // FIXME
#  define RADIO_GPIO_DATA  4 // FIXME
#  define RADIO_GPIO_LED   5 // FIXME
#endif
//----------------------------------------------------------------------------
// SPI max speed [Hz]
#define RADIO_SPI_SPEED 20000000 // 20 MHz 
//----------------------------------------------------------------------------
extern sx127x_t radio;
extern int radio_stop;
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
//----------------------------------------------------------------------------
// on/off DATA line
void radio_data_on(bool on);
//-----------------------------------------------------------------------------
// on/off LED
void radio_led_on(bool on);
//-----------------------------------------------------------------------------
// blink LED
void radio_blink_led();
//-----------------------------------------------------------------------------
// hard reset SX127x radio module
void radio_reset();
//----------------------------------------------------------------------------
// init SX127x radio module 
void radio_init();
//-----------------------------------------------------------------------------
// free SX127x radio module
void radio_free();
//-----------------------------------------------------------------------------
// SPI exchange wrapper function (return number or RX bytes)
int radio_spi_exchange(
  u8_t       *rx_buf, // RX buffer
  const u8_t *tx_buf, // TX buffer
  u8_t len,           // number of bytes
  void *context);     // optional SPI context or NULL
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif // __cplusplus
//----------------------------------------------------------------------------
#endif // RADIO_H

/*** end of "radio.h" file ***/


