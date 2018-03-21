/*
 * -*- coding: UTF8 -*-
 * Semtech SX127x famaly chips driver
 * File: "sx127x.h"
 */

#ifndef SX127X_H
#define SX127X_H
//-----------------------------------------------------------------------------
// SX127x class mode
#define SX127X_LORA 0
#define SX127X_FSK  1
#define SX127X_OOK  2

//-----------------------------------------------------------------------------
// common error codes (return values)
#define SX127X_ERR_NONE 0 // no error, success
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
// common SX127x configuration
typedef struct sx127x_pars_common_ {
   u8_t mode;       // mode: 0 - LoRa, 1 - FSK, 2 - OOK
  u32_t freq;       // frequency [Hz] (434000000 -> 434 MHz)
   u8_t out_power;  // `OutputPower` level 0...15
   u8_t max_power;  // `MaxPower` parametr 0...7 (7 by default)
   bool pa_boost;   // true - use PA_BOOT out pin, false - use RFO out pin
   bool high_power; // if true then add +3 dB to power on PA_BOOST output pin
   bool crc;        // CRC in packet modes false - off, true - on
} sx127x_pars_common_t;
//----------------------------------------------------------------------------
// LoRaTM mode SX127x configuration
typedef struct sx127x_pars_lora_ {
  u32_t bw;         // Bandwith [Hz]: 7800...500000 (125000 -> 125 kHz)
   u8_t sf;         // Spreading Facror: 6..12
   u8_t cr;         // Code Rate: 5...8
   i8_t ldro;       // Low Data Rate Optimize: 1 - on, 0 - off, -1 - automatic
   u8_t sw;         // Sync Word (allways 0x12)
  u16_t preamble;   // Size of preamble: 6...65535 (8 by default)
   bool impl_hdr;   // Implicit Header off/on
   //bool rx_single;  // `RsSinleOn`
   //bool freq_hop;   // `FreqHopOn`
   //u8_t hop_period; // `HopPeriod`                 
} sx127x_pars_lora_t;
//----------------------------------------------------------------------------
// FSK/OOK mode SX127x configuration
typedef struct sx127x_pars_fskook_ {
  u16_t bitrate; // bitrate [bit/s] (4800 bit/s for example)
  u16_t fdev;    // frequency deviation [Hz] (5000 Hz for example)
  u32_t rx_bw;   // RX  bandwidth [Hz]: 2600...250000 kHz (10400 -> 10.4 kHz)
  u32_t afc_bw;  // AFC bandwidth [Hz]: 2600...250000 kHz (2600  ->  2.6 kHz)
   u8_t afc;     // AFC on/off: 0 - off, 1 - on
   u8_t fixed;   // 0 - variable packet size, 1 - fixed packet size
   u8_t dcfree;  // DC free method: 0 - None, 1 - Manchester, 2 - Whitening
} sx127x_pars_fskook_t;
//----------------------------------------------------------------------------
// SX127x class pivate data
typedef struct sx127x_ {
  u32_t freq; // frequency [Hz] (434000000 -> 434 MHz)

} sx127x_t;
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
//----------------------------------------------------------------------------
int sx127x_init(sx127x_t *self);
                
//----------------------------------------------------------------------------
void sx127x_free(sx127x_t *self);
//----------------------------------------------------------------------------
//...
//----------------------------------------------------------------------------
void sx127x_irq_handler(sx127x_t *self);
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif // __cplusplus
//----------------------------------------------------------------------------
#endif // SX127X_H

/*** end of "sx127x.h" file ***/


