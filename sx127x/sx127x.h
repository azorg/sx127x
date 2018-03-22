/*
 * -*- coding: UTF8 -*-
 * Semtech SX127x famaly chips driver
 * File: "sx127x.h"
 */

#ifndef SX127X_H
#define SX127X_H
//-----------------------------------------------------------------------------
// common error codes (return values)
#define SX127X_ERR_NONE     0 // no error, success
#define SX127X_ERR_VERSION -1 // error of crystal revision
//...

//----------------------------------------------------------------------------
#ifdef SX127X_DEBUG
#  include <stdio.h> // fprintf()
#    define SX127X_DBG(fmt, arg...) fprintf(stderr, "SX127X: " fmt "\n", ## arg)
#else
#  define SX127X_DBG(fmt, ...) // debug output off
#endif // SX127X_DEBUG
//----------------------------------------------------------------------------
// integer types
typedef unsigned char   u8_t;
typedef          char   i8_t;
typedef unsigned short u16_t;
typedef          short i16_t;
typedef unsigned long  u32_t;
typedef          long  i32_t;
//----------------------------------------------------------------------------
// bool type
typedef u8_t bool;
#define true  1
#define false 0
//----------------------------------------------------------------------------
// float types
typedef float f32_t;
//typedef double f64_t;
//----------------------------------------------------------------------------
// SX127x radio mode
typedef enum {
  SX127X_LORA = 1, // LoRaTM 
  SX127X_FSK  = 2, // FSK
  SX127X_OOK  = 3  // OOK
} sx127x_mode_t;
//----------------------------------------------------------------------------
// SX127x configuration structure
typedef struct sx127x_pars_ {
  // common pars:
  u32_t freq;       // frequency [Hz] (434000000 -> 434 MHz)
   u8_t out_power;  // `OutputPower` level 0...15
   u8_t max_power;  // `MaxPower` parametr 0...7 (7 by default)
   bool pa_boost;   // true - use PA_BOOT out pin, false - use RFO out pin
   bool high_power; // if true then add +3 dB to power on PA_BOOST output pin
   bool crc;        // CRC in packet modes false - off, true - on

  // LoRaTM mode pars:
  u32_t bw;         // Bandwith [Hz]: 7800...500000 (125000 -> 125 kHz)
   u8_t sf;         // Spreading Facror: 6..12
   u8_t cr;         // Code Rate: 5...8
   i8_t ldro;       // Low Data Rate Optimize: 1 - on, 0 - off, -1 - automatic
   u8_t sw;         // Sync Word (allways 0x12)
  u16_t preamble;   // Size of preamble: 6...65535 (8 by default)
   bool impl_hdr;   // Implicit Header on/off
   //bool rx_single;  // `RsSinleOn`
   //bool freq_hop;   // `FreqHopOn`
   //u8_t hop_period; // `HopPeriod`                 
 
  // FSK/OOK mode pars:
  u16_t bitrate; // bitrate [bit/s] (4800 bit/s for example)
  u16_t fdev;    // frequency deviation [Hz] (5000 Hz for example)
  u32_t rx_bw;   // RX  bandwidth [Hz]: 2600...250000 kHz (10400 -> 10.4 kHz)
  u32_t afc_bw;  // AFC bandwidth [Hz]: 2600...250000 kHz (2600  ->  2.6 kHz)
   bool afc;     // AFC on/off
   bool fixed;   // false - variable packet size, true - fixed packet size
   u8_t dcfree;  // DC free method: 0 - None, 1 - Manchester, 2 - Whitening

} sx127x_pars_t;
//----------------------------------------------------------------------------
// SX127x class pivate data
typedef struct sx127x_ sx127x_t;
struct sx127x_ {
  sx127x_mode_t mode; // 1 - LoRa, 2 - FSK, 3 - OOK
  
  void (*spi_cs)(bool cs); // SPI crystal select function                          
  
  int (*spi_exchange)( // SPI exchange function
    u8_t       *rx_buf, // RX buffer
    const u8_t *tx_buf, // TX buffer
    u8_t len,           // number of bites
    void *context);     // optional SPI context or NULL
  
  void (*on_receive)( // receive callback or NULL
    sx127x_t *self,     // pointer to sx127x_t object
    u8_t *payload,      // payload data
    u8_t payload_size,  // payload size
    bool crc,           // CRC ok/false
    void *context);     // optional context
  
  sx127x_pars_t pars;         // configuration parameters
  void *spi_exchange_context; // optional SPI exchange context
  void *on_receive_context;   // optional on_receive() context
};
//----------------------------------------------------------------------------
// default SX127x radio module configuration
extern sx127x_pars_t sx127x_pars_default;
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
//----------------------------------------------------------------------------
// init SX127x radio module
int sx127x_init(
  sx127x_t *self,
  sx127x_mode_t mode, // radio mode: 1 - LoRa, 2 - FSK, 3 - OOK

  void (*spi_cs)(bool cs), // SPI crystal select function                          

  int (*spi_exchange)( // SPI exchange function (return number or RX bytes)
    u8_t       *rx_buf, // RX buffer
    const u8_t *tx_buf, // TX buffer
    u8_t len,           // number of bytes
    void *context),     // optional SPI context or NULL

  void (*on_receive)(  // receive callback or NULL
    sx127x_t *self,     // pointer to sx127x_t object
    u8_t *payload,      // payload data
    u8_t payload_size,  // payload size
    bool crc,           // CRC ok/false
    void *context),     // optional context

  const sx127x_pars_t *pars,  // configuration parameters or NULL
  void *spi_exchange_context, // optional SPI exchange context
  void *on_receive_context);  // optional on_receive() context
//----------------------------------------------------------------------------
// setup SX127x radio module (uses from sx127x_init())
int sx127x_set_pars(
  sx127x_t *self,
  const sx127x_pars_t *pars); // configuration parameters or NULL
//----------------------------------------------------------------------------
// free SX127x radio module
void sx127x_free(sx127x_t *self);
//----------------------------------------------------------------------------
// get SX127x crystal revision
u8_t sx127x_version(sx127x_t *self);
//----------------------------------------------------------------------------
// set mode in `RegOpMode` register
void sx127x_set_mode(sx127x_t *self, u8_t mode);
//----------------------------------------------------------------------------
// get mode from `RegOpMode` register
u8_t sx127x_get_mode(sx127x_t *self);
//----------------------------------------------------------------------------
// switch SX127x radio module to LoRaTM mode
void sx127x_lora(sx127x_t *self);
//----------------------------------------------------------------------------
// check LoRa (or FSK/OOK) mode
bool sx12x_is_lora(sx127x_t *self);
//----------------------------------------------------------------------------
// switch SX127x radio module to FSK mode
void sx127x_fsk(sx127x_t *self);
//----------------------------------------------------------------------------
// switch SX127x radio module to OOK mode
void sx127x_ook(sx127x_t *self);
//----------------------------------------------------------------------------
//...
//----------------------------------------------------------------------------
// IRQ handler on DIO0 pin
void sx127x_irq_handler(sx127x_t *self);
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif // __cplusplus
//----------------------------------------------------------------------------
#endif // SX127X_H

/*** end of "sx127x.h" file ***/


