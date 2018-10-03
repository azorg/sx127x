/*
 * -*- coding: UTF8 -*-
 * Semtech SX127x famaly chips driver
 * File: "sx127x.h"
 */

#ifndef SX127X_H
#define SX127X_H
//-----------------------------------------------------------------------------
#ifndef SX127X_MAX_PACKET
#define SX127X_MAX_PACKET 256
#endif
//-----------------------------------------------------------------------------
#define SX127X_USE_LORA   // use LoRaTM mode
#define SX127X_USE_FSKOOK // use FSK/OOK mode
#define SX127X_USE_EXTRA  // use some extra funtions
//-----------------------------------------------------------------------------
// limit arguments
#define SX127X_LIMIT(x, min, max) \
  ((x) > (max) ? (max) : ((x) < (min) ? (min) : (x)))

#define SX127X_MIN(x, y) ((x) < (y) ? (x) : (y))

#define SX127X_MAX(x, y) ((x) > (y) ? (x) : (y))
//-----------------------------------------------------------------------------
// common error codes (return values)
#define SX127X_ERR_NONE      0 // no error, success
#define SX127X_ERR_VERSION  -1 // error of crystal revision
#define SX127X_ERR_BAD_SIZE -2 // bad size of send packet (<=0)

//----------------------------------------------------------------------------
//#define SX127X_DEBUG
#ifdef SX127X_DEBUG
#  include <stdio.h> // fprintf()
#    define SX127X_DBG(fmt, arg...) fprintf(stderr, "SX127x: " fmt "\r\n", ## arg)
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
// SX127x radio mode
typedef enum {
  SX127X_LORA = 0, // LoRaTM 
  SX127X_FSK  = 1, // FSK
  SX127X_OOK  = 2  // OOK
} sx127x_mode_t;
//----------------------------------------------------------------------------
// SX127x configuration structure
typedef struct sx127x_pars_ {
  // common pars:
  u32_t freq;       // frequency [Hz] (434000000 -> 434 MHz)
  u8_t  out_power;  // `OutputPower` level 0...15
  u8_t  max_power;  // `MaxPower` parametr 0...7 (7 by default)
  bool  pa_boost;   // true - use PA_BOOT out pin, false - use RFO out pin
  bool  high_power; // if true then add +3 dB to power on PA_BOOST output pin
  u8_t  ocp;        // OCP trimmer [mA] (0 <=> OCP off)
  bool  crc;        // CRC in packet modes false - off, true - on

#ifdef SX127X_USE_LORA
  // LoRaTM mode pars:
  u32_t bw;         // Bandwith [Hz]: 7800...500000 (125000 -> 125 kHz)
  u8_t  cr;         // Code Rate: 5...8
  u8_t  sf;         // Spreading Facror: 6..12
  i8_t  ldro;       // Low Data Rate Optimize: 1 - on, 0 - off, -1 - automatic
  u8_t  sw;         // Sync Word (allways 0x12)
  u16_t preamble;   // Size of preamble: 6...65535 (8 by default)
  bool impl_hdr;    // true - implicit header mode, false - explicit
  //bool freq_hop;    // `FreqHopOn`
  //u8_t hop_period;  // `HopPeriod`                 
#endif

#ifdef SX127X_USE_FSKOOK
  // FSK/OOK mode pars:
  u32_t bitrate; // bitrate [bit/s] (4800 bit/s for example)
  u32_t fdev;    // frequency deviation [Hz] (5000 Hz for example)
  u32_t rx_bw;   // RX  bandwidth [Hz]: 2600...250000 kHz (10400 -> 10.4 kHz)
  u32_t afc_bw;  // AFC bandwidth [Hz]: 2600...250000 kHz (2600  ->  2.6 kHz)
  bool  afc;     // AFC on/off
  bool  fixed;   // true - fixed packet length, false - variable length
  u8_t  dcfree;  // DC free method: 0 - None, 1 - Manchester, 2 - Whitening
#endif
} sx127x_pars_t;
//----------------------------------------------------------------------------
// SX127x class pivate data
typedef struct sx127x_ sx127x_t;
struct sx127x_ {
  sx127x_mode_t mode; // radio mode: SX127X_LORA, SX127X_FSK, SX127X_OOK
  u32_t freq;         // frequency [Hz] (434000000 -> 434 MHz)
  bool pa_boost;      // true - use PA_BOOT out pin, false - use RFO out pin
  bool crc;           // CRC in packet modes: false - off, true - on
#ifdef SX127X_USE_LORA
  bool impl_hdr;      // true - implicit header mode, false - explicit
#endif
#ifdef SX127X_USE_FSKOOK
  bool fixed;         // true - fixed packet length, false - variable length
#endif

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
  
  void *spi_exchange_context; // optional SPI exchange context
  void *on_receive_context;   // optional on_receive() context

  u8_t payload[SX127X_MAX_PACKET]; // payload receiver buffer
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
  sx127x_mode_t mode,  // radio mode: SX127X_LORA, SX127X_FSK, SX127X_OOK

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
// free SX127x radio module
void sx127x_free(sx127x_t *self);
//----------------------------------------------------------------------------
// set callback on receive packet (Lora/FSK/OOK)
void sx127x_on_receive(
  sx127x_t *self,
  void (*on_receive)(        // receive callback or NULL
    sx127x_t *self,            // pointer to sx127x_t object
    u8_t *payload,             // payload data
    u8_t payload_size,         // payload size
    bool crc,                  // CRC ok/false
    void *context),            // optional context
  void *on_receive_context); // optional on_receive() context
//-----------------------------------------------------------------------------
// write SX127x 8-bit register to SPI
void sx127x_write_reg(sx127x_t *self, u8_t address, u8_t value);
//----------------------------------------------------------------------------
// read SX127x 8-bit register from SPI
u8_t sx127x_read_reg(sx127x_t *self, u8_t address);
//----------------------------------------------------------------------------
// setup SX127x radio module (uses from sx127x_init())
void sx127x_set_pars(
  sx127x_t *self,
  sx127x_mode_t mode, // radio mode: SX127X_LORA, SX127X_FSK, SX127X_OOK
  const sx127x_pars_t *pars); // configuration parameters or NULL
//----------------------------------------------------------------------------
// return current mode (SX127X_LORA, SX127X_FSK, SX127X_OOK)
sx127x_mode_t sx127x_mode(sx127x_t *self);
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
#ifdef SX127X_USE_LORA
// switch to LoRa mode
void sx127x_lora(sx127x_t *self);
#endif
//----------------------------------------------------------------------------
#ifdef SX127X_USE_EXTRA
// check LoRa (or FSK/OOK) mode
bool sx127x_is_lora(sx127x_t *self);
#endif
//----------------------------------------------------------------------------
#ifdef SX127X_USE_FSKOOK
// switch to FSK mode
void sx127x_fsk(sx127x_t *self);
//----------------------------------------------------------------------------
// switch to OOK mode
void sx127x_ook(sx127x_t *self);
#endif
//----------------------------------------------------------------------------
// switch to Sleep mode
void sx127x_sleep(sx127x_t *self);
//----------------------------------------------------------------------------
// switch to Stanby mode
void sx127x_standby(sx127x_t *self);
//----------------------------------------------------------------------------
// switch to TX mode
void sx127x_tx(sx127x_t *self);
//----------------------------------------------------------------------------
// switch to RX (continuous) mode
void sx127x_rx(sx127x_t *self);
//----------------------------------------------------------------------------
#if defined(SX127X_USE_LORA) && defined(SX127X_USE_EXTRA)
// switch to CAD (LoRa) mode
void sx127x_cad(sx127x_t *self);
#endif
//----------------------------------------------------------------------------
// set RF frequency [Hz]
u32_t sx127x_set_frequency(sx127x_t *self, u32_t freq);
//----------------------------------------------------------------------------
#ifdef SX127X_USE_EXTRA
// get RF frequency [Hz]
u32_t sx127x_get_frequency(sx127x_t *self);
#endif
//----------------------------------------------------------------------------
// update band after change RF frequency from one band to another
void sx127x_update_band(sx127x_t *self);
//----------------------------------------------------------------------------
// set LNA boost on/off (only for high frequency band)
void sx127x_set_lna_boost(sx127x_t *self, bool lna_boost);
//----------------------------------------------------------------------------
// get current RX gain code [1..6] from `RegLna` (1 - maximum gain)
u8_t sx127x_get_rx_gain(sx127x_t *self);
//----------------------------------------------------------------------------
// get RSSI [dB]
i16_t sx127x_get_rssi(sx127x_t *self);
//----------------------------------------------------------------------------
#ifdef SX127X_USE_LORA
// get packet RSSI [dB]
i16_t sx127x_get_pkt_rssi(sx127x_t *self);
//----------------------------------------------------------------------------
// get SNR [dB] (LoRa)
i16_t sx127x_get_snr(sx127x_t *self);
#endif
//----------------------------------------------------------------------------
// set TX power levels, select/deselect PA_BOOST pin
// 1. if PA_BOOST pin selected then:
//    Pout = 2 + out_power = 2...17 dBm
//    (Note: "HighPower" add +3 dBm to PA_BOOST pin)
// 2. if RFO pin selected then:
//    Pmax = 10.8 + 0.6 * max_power = 10.8...15 dBm
//    Pout = Pmax - 15 + out_power  = -4.2...15 dBm
i8_t sx127x_set_power(sx127x_t *self,
                      bool pa_boost,   // `PA_BOOST`    true/false
                      u8_t out_power,  // `OutputPower` 0...15 dB
                      u8_t max_power); // `MaxPower`    0...7 (7=default)
//----------------------------------------------------------------------------
#ifdef SX127X_USE_EXTRA
// set TX output power: -4...+15 dBm on RFO or +2...+17 dBm on PA_BOOST
i8_t sx127x_set_power_dbm(sx127x_t *self, i8_t dbm);
//----------------------------------------------------------------------------
// set high power (+3 dB) on PA_BOOST pin (up to +20 dBm output power)
void sx127x_set_high_power(sx127x_t *self, bool on);
//----------------------------------------------------------------------------
// set trimming of OCP current (45...240 mA)
// (note: you must set OCP trmimmer if set high power, look datasheet)
void sx127x_set_ocp(sx127x_t *self, u8_t trim_mA, bool on);
//----------------------------------------------------------------------------
// set PA rise/fall time of ramp code 0..15 (FSK/LoRa), default is 9
// set modulation shaping code (0..3 in FSK, 0...2 in OOK), default is 0
void sx127x_set_ramp(sx127x_t *self, u8_t shaping, u8_t ramp);
//----------------------------------------------------------------------------
// set PLL bandwidth 0=75, 1=150, 2=225, 3=300 kHz (LoRa/FSK/OOK), default 3
void sx127x_set_pll_bw(sx127x_t *self, u8_t bw);
#endif
//----------------------------------------------------------------------------
// enable/disable CRC LoRa/FSK/OOK, set/unset `CrcAutoClearOff` (FSK/OOK mode)
void sx127x_enable_crc(sx127x_t *self, bool crc, bool crcAutoClearOff);
//----------------------------------------------------------------------------
#ifdef SX127X_USE_LORA
// set signal Bandwidth 7800...500000 Hz (LoRa)
void sx127x_set_bw(sx127x_t *self, u32_t bw);
//----------------------------------------------------------------------------
// set Coding Rate 5...8 (LoRa)
void sx127x_set_cr(sx127x_t *self, u8_t cr);
//----------------------------------------------------------------------------
// on/off "ImplicitHeaderMode" (LoRa)
// (with SF=6 implicit header mode is the only mode of operation possible)
void sx127x_impl_hdr(sx127x_t *self, bool impl_hdr);
//----------------------------------------------------------------------------
// set Spreading Factor 6...12 (LoRa)
void sx127x_set_sf(sx127x_t *self, u8_t sf);
//----------------------------------------------------------------------------
// set Low Data Rate Optimisation (LoRa)
void sx127x_set_ldro(sx127x_t *self, bool ldro);
//----------------------------------------------------------------------------
// set preamble length [6...65535] (LoRa)
void sx127x_set_preamble(sx127x_t *self, u16_t length);
//----------------------------------------------------------------------------
// set Sync Word (LoRa)
void sx127x_set_sw(sx127x_t *self, u8_t sw);
//----------------------------------------------------------------------------
// invert IQ channels (LoRa)
void sx127x_invert_iq(sx127x_t *self, bool invert);
#endif
//----------------------------------------------------------------------------
#ifdef SX127X_USE_FSKOOK
// select Continuous mode, must use DIO2->DATA, DIO1->DCLK (FSK/OOK)
void sx127x_continuous(sx127x_t *self, bool on);
//----------------------------------------------------------------------------
// RSSI and IQ callibration (FSK/OOK)
void sx127x_rx_calibrate(sx127x_t *self);
//----------------------------------------------------------------------------
// set bitrate (>= 500 bit/s) (FSK/OOK)
u32_t sx127x_set_bitrate(sx127x_t *self, u32_t bitrate);
//----------------------------------------------------------------------------
// set frequency deviation [Hz] (FSK)
// (note: must be set between 600 Hz and 200 kHz)
void sx127x_set_fdev(sx127x_t *self, u32_t fdev);
//----------------------------------------------------------------------------
// set RX bandwidth [Hz] (FSK/OOK)
void sx127x_set_rx_bw(sx127x_t *self, u32_t bw);
//----------------------------------------------------------------------------
// set AFC bandwidth [Hz] (FSK/OOK)
void sx127x_set_afc_bw(sx127x_t *self, u32_t bw);
//----------------------------------------------------------------------------
// enable/disable AFC (FSK/OOK)
void sx127x_set_afc(sx127x_t *self, bool afc);
//----------------------------------------------------------------------------
// set Fixed or Variable packet mode (FSK/OOK)
void sx127x_set_fixed(sx127x_t *self, bool fixed);
//----------------------------------------------------------------------------
// set DcFree mode: 0=Off, 1=Manchester, 2=Whitening (FSK/OOK)
void sx127x_set_dcfree(sx127x_t *self, u8_t dcfree);
//----------------------------------------------------------------------------
// on/off fast frequency PLL hopping (FSK/OOK)
void sx127x_set_fast_hop(sx127x_t *self, bool on);
#endif
//----------------------------------------------------------------------------
// send packet (LoRa/FSK/OOK)
// fixed - implicit header mode (LoRa), fixed packet length (FSK/OOK)
i16_t sx127x_send(sx127x_t *self, const u8_t *data, i16_t size, bool fixed);
//----------------------------------------------------------------------------
// go to RX mode; wait callback by interrupt (LoRa/FSK/OOK)
// LoRa:    if pkt_len = 0 then explicit header mode, else - implicit
// FSK/OOK: if pkt_len = 0 then variable packet length, else - fixed
void sx127x_receive(sx127x_t *self, i16_t pkt_len);
//----------------------------------------------------------------------------
// IRQ handler on DIO0 pin
void sx127x_irq_handler(sx127x_t *self);
//----------------------------------------------------------------------------
#if defined(SX127X_USE_LORA) && defined(SX127X_USE_EXTRA)
// enable/disable interrupt by RX done for debug (LoRa)
void sx127x_enable_rx_irq(sx127x_t *self, bool enable);
#endif
//----------------------------------------------------------------------------
#ifdef SX127X_USE_EXTRA
// get IRQ flags for debug
u16_t sx127x_get_irq_flags(sx127x_t *self);
//----------------------------------------------------------------------------
// dump registers for debug
void sx127x_dump(sx127x_t *self);
#endif
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif // __cplusplus
//----------------------------------------------------------------------------
#endif // SX127X_H

/*** end of "sx127x.h" file ***/


