/*
 * -*- coding: UTF8 -*-
 * Semtech SX127x famaly chips driver
 * File: "sx127x.c"
 */

//-----------------------------------------------------------------------------
#include <stdio.h>      // NULL
#include "sx127x.h"     // `sx127x_t`
#include "sx127x_def.h" // SX127x define's
//-----------------------------------------------------------------------------
// default SX127x radio module configuration
sx127x_pars_t sx127x_pars_default = {
  // common:
  434000000, // frequency [Hz] (434000000 -> 434 MHz)
  15,        // `OutputPower` level 0...15
  7,         // `MaxPower` parametr 0...7 (7 by default)
  true,      // true - use PA_BOOT out pin, false - use RFO out pin
  false,     // if true then add +3 dB to power on PA_BOOST output pin
  200,       // OCP trimmer [mA] (0 <=> OCP off)
  true,      // CRC in packet modes false - off, true - on

  // LoRaTM mode:
  125000, // Bandwith [Hz]: 7800...500000 (125000 -> 125 kHz)
  5,      // Code Rate: 5...8
  9,      // Spreading Facror: 6..12
  -1,     // Low Data Rate Optimize: 1 - on, 0 - off, -1 - automatic
  0x12,   // Sync Word (allways 0x12)
  8,      // Size of preamble: 6...65535 (8 by default)
  false,  // true - implicit header mode, false - explicit
 
  // FSK/OOK mode:
  4800,  // bitrate [bit/s] (4800 bit/s for example)
  5000,  // frequency deviation [Hz] (5000 Hz for example)
  10400, // RX  bandwidth [Hz]: 2600...250000 kHz (10400 -> 10.4 kHz)
  2600,  // AFC bandwidth [Hz]: 2600...250000 kHz (2600  ->  2.6 kHz)
  true,  // AFC on/off
  false, // true - fixed packet length, false - variable length
  0      // DC free method: 0 - None, 1 - Manchester, 2 - Whitening
};
//-----------------------------------------------------------------------------
// BandWith table [kHz] (LoRa)
static u32_t sx127x_bw_tbl[] = BW_TABLE; // Hz

// RX BandWith table (FSK/OOK)
typedef struct sx127x_rf_bw_tbl_ {
  u8_t  m;  // 0...2
  u8_t  e;  // 1...7
  u32_t bw; // 2600...250000 Hz
} sx127x_rf_bw_tbl_t;

static sx127x_rf_bw_tbl_t sx127x_rf_bw_tbl[] = RX_BW_TABLE;
//-----------------------------------------------------------------------------
// get index of bandwidth (LoRa mode), bandwidth in Hz
static void sx127x_bw_pack(u32_t *bw, u8_t *ix)
{
  int i, n = sizeof(sx127x_bw_tbl) / sizeof(u32_t);
  *ix = (u8_t) (n - 1);
  for (i = 0; i < n; i++)
  {
    if (*bw <= sx127x_bw_tbl[i])
    {
      *ix = (u8_t) i;
      break;
    }
  }
  *bw = sx127x_bw_tbl[*ix];
}
//-----------------------------------------------------------------------------
// pack RX bandwidth in Hz to mant/exp micro float format (FSK/OOK mode)
static void sx127x_rx_bw_pack(u32_t *bw, u8_t *m, u8_t *e)
{
  int i, n = sizeof(sx127x_rf_bw_tbl) / sizeof(sx127x_rf_bw_tbl_t);

  for (i = 0; i < n; i++)
  {
    if (*bw <= sx127x_rf_bw_tbl[i].bw)
    {
      *bw = sx127x_rf_bw_tbl[i].bw;
      *m  = sx127x_rf_bw_tbl[i].m;
      *e  = sx127x_rf_bw_tbl[i].e;
      return;
    }
  }

  *bw = sx127x_rf_bw_tbl[n - 1].bw;
  *m  = sx127x_rf_bw_tbl[n - 1].m;
  *e  = sx127x_rf_bw_tbl[n - 1].e;
}
//-----------------------------------------------------------------------------
// init SX127x radio module
int sx127x_init(
  sx127x_t *self,
  sx127x_mode_t mode, // radio mode: SX127X_LORA, SX127X_FSK, SX127X_OOK

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
  void *on_receive_context)   // optional on_receive() context
{
  if (mode != SX127X_FSK && mode != SX127X_OOK) mode = SX127X_LORA;

  self->mode         = mode;
  self->spi_exchange = spi_exchange;
  self->on_receive   = on_receive;

  self->spi_exchange_context = spi_exchange_context;
  self->on_receive_context   = on_receive_context;

  SX127X_DBG("init radio module in %s mode",
             mode == SX127X_LORA ? "LoRaTM" :
             mode == SX127X_FSK  ? "FSK"    :
             mode == SX127X_OOK  ? "OOK"    :
                                   "Unknown");

  return sx127x_set_pars(self, pars);
}
//----------------------------------------------------------------------------
// free SX127x radio module
void sx127x_free(sx127x_t *self)
{
  // do nothing
  sx127x_sleep(self);
}
//----------------------------------------------------------------------------
// set callback on receive packet (Lora/FSK/OOK)
void sx127x_on_receive(
  sx127x_t *self,
  void (*on_receive)(       // receive callback or NULL
    sx127x_t *self,           // pointer to sx127x_t object
    u8_t *payload,            // payload data
    u8_t payload_size,        // payload size
    bool crc,                 // CRC ok/false
    void *context),           // optional context
  void *on_receive_context) // optional on_receive() context
{
  self->on_receive         = on_receive;
  self->on_receive_context = on_receive_context;
}
//----------------------------------------------------------------------------
// SPI transfer help function
u8_t sx127x_spi_transfer(sx127x_t *self, u8_t address, u8_t value)
{
  u8_t tx_buf[2], rx_buf[2];
  
  // prepare TX buffer
  tx_buf[0] = address;
  tx_buf[1] = value;

  // transfe data over SPI
  self->spi_exchange(rx_buf, tx_buf, 2, self->spi_exchange_context);
  
  return rx_buf[1];
}
//-----------------------------------------------------------------------------
// read SX127x 8-bit register from SPI
u8_t sx127x_read_reg(sx127x_t *self, u8_t address)
{
  return sx127x_spi_transfer(self, address & 0x7F, 0xFF);
}
//-----------------------------------------------------------------------------
// write SX127x 8-bit register to SPI
void sx127x_write_reg(sx127x_t *self, u8_t address, u8_t value)
{
  sx127x_spi_transfer(self, address | 0x80, value);
}
//----------------------------------------------------------------------------
// setup SX127x radio module (uses from sx127x_init())
i16_t sx127x_set_pars(
  sx127x_t *self,
  const sx127x_pars_t *pars) // configuration parameters or NULL
{
  u8_t version;
  i16_t retv = SX127X_ERR_NONE;
  
  if (pars == (sx127x_pars_t*) NULL)
    pars = &sx127x_pars_default; // use default pars

  // check version
  version = sx127x_version(self);
  if (version == 0x12)
  {
    SX127X_DBG("selicon revision = 0x%02X [OK]", version);
  }
  else
  {
    SX127X_DBG("selicon revision = 0x%02X [FAIL]", version);
    retv = SX127X_ERR_VERSION;
  }

  // switch mode
  if      (self->mode == SX127X_FSK) sx127x_fsk(self);  // FSK
  else if (self->mode == SX127X_OOK) sx127x_ook(self);  // OOK
  else                               sx127x_lora(self); // LoRa

  // config RF frequency
  sx127x_set_frequency(self, pars->freq);

  // update band RF frequency
  sx127x_update_band(self);

  // set LNA boost: `LnaBoostHf`->3 (Boost on, 150% LNA current)
  sx127x_set_lna_boost(self, true);

  // set output power level
  sx127x_set_power_ex(self, pars->pa_boost,
                            pars->out_power, pars->max_power);

  // set high power (+3 dB) on PA_BOOST pin (up to +20 dBm output power)
  sx127x_set_high_power(self, pars->high_power);

  // set trimming of OCP current (45...240 mA)
  if (pars->ocp)
    sx127x_set_ocp(self, pars->ocp, true);
  else
    sx127x_set_ocp(self, 120, false);

  // enable/disable CRC (`CrcAutoClearOff`=1)
  sx127x_enable_crc(self, pars->crc, true);

  if (self->mode == SX127X_LORA)
  { // set LoRaTM options
    sx127x_set_bw(self, pars->bw); // BW [Hz]
    sx127x_set_cr(self, pars->cr); // CR
    sx127x_set_sf(self, pars->sf); // SF

    if (pars->sf <= 6)
      sx127x_impl_hdr(self, true); // `ImplicitHeaderMode`=1
    else
      sx127x_impl_hdr(self, pars->impl_hdr); // `ImplicitHeaderMode`
    
    if (pars->ldro == 0)
      sx127x_set_ldro(self, false);          // LDRO off
    else if (pars->ldro > 0)
      sx127x_set_ldro(self, true);           // LDRO on
    else // pars->ldro < 0
      sx127x_set_ldro(self, pars->sf >= 10); // LDRO auto
   
    sx127x_set_preamble(self, pars->preamble); // preamble length
    sx127x_set_sw(self, pars->sw);             // Sync Word

    // set AGC auto on (internal AGC loop)
    sx127x_write_reg(self, REG_MODEM_CONFIG_3,
      sx127x_read_reg(self, REG_MODEM_CONFIG_3) | 0x04); // `AgcAutoOn`

    // set base addresses
    sx127x_write_reg(self, REG_FIFO_TX_BASE_ADDR, FIFO_TX_BASE_ADDR);
    sx127x_write_reg(self, REG_FIFO_RX_BASE_ADDR, FIFO_RX_BASE_ADDR);

    // set DIO0 mapping (`RxDone`)
    sx127x_write_reg(self, REG_DIO_MAPPING_1, 0x00);
    
    // set maximum payload length
    sx127x_write_reg(self, REG_MAX_PAYLOAD_LEN, MAX_PKT_LENGTH);
  }
  else
  { // set FSK/OOK options
     sx127x_continuous(self,  false); // packet mode by default 
     sx127x_set_bitrate(self, pars->bitrate); // bit/s
     sx127x_set_fdev(self,    pars->fdev);    // [Hz]
     sx127x_set_rx_bw(self,   pars->rx_bw);   // Hz
     sx127x_set_afc_bw(self,  pars->afc_bw);  // Hz
     sx127x_set_afc(self,     pars->afc);     // on/off
     sx127x_set_fixed(self,   pars->fixed);   // on/off
     sx127x_set_dcfree(self,  pars->dcfree);  // 0, 1 or 2
    
     sx127x_write_reg(self, REG_RSSI_TRESH, 0xFF); // default
     sx127x_write_reg(self, REG_PREAMBLE_LSB, 8);  // 3 by default

     sx127x_write_reg(self, REG_SYNC_VALUE_1, 0x69); // 0x01 by default
     sx127x_write_reg(self, REG_SYNC_VALUE_2, 0x81); // 0x01 by default
     sx127x_write_reg(self, REG_SYNC_VALUE_3, 0x7E); // 0x01 by default
     sx127x_write_reg(self, REG_SYNC_VALUE_4, 0x96); // 0x01 by default

     // set `DataMode` to Packet (and reset PayloadLength(10:8) to 0)
     sx127x_write_reg(self, REG_PACKET_CONFIG_2, 0x40);

     // set TX start FIFO condition
     sx127x_write_reg(self, REG_FIFO_THRESH, TX_START_FIFO_NOEMPTY);

     // set DIO0 mapping (by default):
     //    in RxContin - `SyncAddres`
     //    in TxContin - `TxReady`
     //    in RxPacket - `PayloadReady` <- used signal
     //    in TxPacket - `PacketSent`
     sx127x_write_reg(self, REG_DIO_MAPPING_1, 0x00);

     // RSSI and IQ calibrate
     sx127x_rx_calibrate(self);
  }

  sx127x_standby(self);

  return retv;
}
//----------------------------------------------------------------------------
// get SX127x crystal revision
u8_t sx127x_version(sx127x_t *self)
{
  return sx127x_read_reg(self, REG_VERSION);
}
//----------------------------------------------------------------------------
// set mode in `RegOpMode` register
void sx127x_set_mode(sx127x_t *self, u8_t mode)
{
  u8_t reg = sx127x_read_reg(self, REG_OP_MODE);
  reg = (reg & ~MODES_MASK) | mode;
  sx127x_write_reg(self, REG_OP_MODE, reg);
}
//----------------------------------------------------------------------------
// get mode from `RegOpMode` register
u8_t sx127x_get_mode(sx127x_t *self)
{
  return sx127x_read_reg(self, REG_OP_MODE) & MODES_MASK;
}
//----------------------------------------------------------------------------
// switch to LoRa mode
void sx127x_lora(sx127x_t *self)
{
  u8_t mode = sx127x_read_reg(self, REG_OP_MODE); // read mode
  u8_t sleep = (mode & ~MODES_MASK) | MODE_SLEEP;
  sx127x_write_reg(self, REG_OP_MODE, sleep); // go to sleep
  sleep |= MODE_LONG_RANGE;
  mode  |= MODE_LONG_RANGE;
  sx127x_write_reg(self, REG_OP_MODE, sleep); // write "long range" bit
  sx127x_write_reg(self, REG_OP_MODE, mode);  // restore old mode
}
//----------------------------------------------------------------------------
// check LoRa (or FSK/OOK) mode
bool sx127x_is_lora(sx127x_t *self)
{
  u8_t mode = sx127x_read_reg(self, REG_OP_MODE); // read mode
  return (mode & MODE_LONG_RANGE) ? true : false;
}
//----------------------------------------------------------------------------
// switch to FSK mode
void sx127x_fsk(sx127x_t *self)
{
  u8_t mode = sx127x_read_reg(self, REG_OP_MODE); // read mode
  u8_t sleep = (mode & ~MODES_MASK) | MODE_SLEEP;
  sx127x_write_reg(self, REG_OP_MODE, sleep); // go to sleep
  sleep &= ~MODE_LONG_RANGE;
  mode  &= ~MODE_LONG_RANGE;
  sx127x_write_reg(self, REG_OP_MODE, sleep); // reset "long range" bit

  // set FSK mode
  sx127x_write_reg(self, REG_OP_MODE, (mode & ~MODES_MASK2) | MODE_FSK);
}
//----------------------------------------------------------------------------
// switch to OOK mode
void sx127x_ook(sx127x_t *self)
{
  u8_t mode = sx127x_read_reg(self, REG_OP_MODE); // read mode
  u8_t sleep = (mode & ~MODES_MASK) | MODE_SLEEP;
  sx127x_write_reg(self, REG_OP_MODE, sleep); // go to sleep
  sleep &= ~MODE_LONG_RANGE;
  mode  &= ~MODE_LONG_RANGE;
  sx127x_write_reg(self, REG_OP_MODE, sleep); // reseet "long range" bit

  // set OOK mode
  sx127x_write_reg(self, REG_OP_MODE, (mode & ~MODES_MASK2) | MODE_OOK);
}
//----------------------------------------------------------------------------
// switch to Sleep mode
void sx127x_sleep(sx127x_t *self)
{
  sx127x_set_mode(self, MODE_SLEEP);
}
//----------------------------------------------------------------------------
// switch to Stanby mode
void sx127x_standby(sx127x_t *self)
{
  sx127x_set_mode(self, MODE_STDBY);
}
//----------------------------------------------------------------------------
// switch to TX mode
void sx127x_tx(sx127x_t *self)
{
  sx127x_set_mode(self, MODE_TX);
}
//----------------------------------------------------------------------------
// switch to RX (continuous) mode
void sx127x_rx(sx127x_t *self)
{
  sx127x_set_mode(self, MODE_RX_CONTINUOUS);
}
//----------------------------------------------------------------------------
// switch to CAD (LoRa) mode
void sx127x_cad(sx127x_t *self)
{
  sx127x_set_mode(self, MODE_CAD);
}
//----------------------------------------------------------------------------
// set RF frequency [Hz]
u32_t sx127x_set_frequency(sx127x_t *self, u32_t freq)
{
  u32_t f, f1, f2, f11, f12, f21, f22;
  u8_t fmsb, fmid, flsb;

  // FREQ_MAGIC_1 = 8     // arithmetic shift
  // FREQ_MAGIC_2 = 625   // 5**4
  // FREQ_MAGIC_3 = 25    // 5**2
  // FREQ_MAGIC_4 = 15625 // 625*25
  f1  = (freq / FREQ_MAGIC_2) << FREQ_MAGIC_1;
  f2  = (freq % FREQ_MAGIC_2) << FREQ_MAGIC_1;

  f11 = f1 / FREQ_MAGIC_3;
  f12 = f1 % FREQ_MAGIC_3;

  f21 = f2 / FREQ_MAGIC_4;
  f22 = f2 % FREQ_MAGIC_4;

  f1  = f11 + f21;
  f2  = (f12 * (FREQ_MAGIC_4 / FREQ_MAGIC_3) + f22 + (FREQ_MAGIC_4 / 2)) /
	FREQ_MAGIC_4;
 
  f = f1 + f2; // FIXME: check limits

  fmsb = (u8_t)(f >> 16);
  fmid = (u8_t)(f >> 8);
  flsb = (u8_t) f;

  sx127x_write_reg(self, REG_FRF_MSB, fmsb);
  sx127x_write_reg(self, REG_FRF_MID, fmid);
  sx127x_write_reg(self, REG_FRF_LSB, flsb);

  // save RF frequency
  self->freq = ((((u32_t) fmsb) * FREQ_MAGIC_4) << 8) +
                (((u32_t) fmid) * FREQ_MAGIC_4) +
               ((((u32_t) flsb) * FREQ_MAGIC_4 + (1<<7)) >> 8);
  
  SX127X_DBG("set RF frequency to %lu Hz (code=%lu)", self->freq, f);

  return self->freq;
}
//----------------------------------------------------------------------------
// get RF frequency [Hz]
u32_t sx127x_get_frequency(sx127x_t *self)
{
  u32_t fmsb = (u32_t) sx127x_read_reg(self, REG_FRF_MSB);
  u32_t fmid = (u32_t) sx127x_read_reg(self, REG_FRF_MID);
  u32_t flsb = (u32_t) sx127x_read_reg(self, REG_FRF_LSB);
  return ((fmsb * FREQ_MAGIC_4) << 8) +
          (fmid * FREQ_MAGIC_4) +
         ((flsb * FREQ_MAGIC_4 + (1<<7)) >> 8);
}
//----------------------------------------------------------------------------
// update band after change RF frequency from one band to another
void sx127x_update_band(sx127x_t *self)
{
  u8_t mode = sx127x_read_reg(self, REG_OP_MODE);
  if (self->freq < 600000000) // LF <= 525 < _600_ < 779 <= HF [MHz]
    mode |=  MODE_LOW_FREQ_MODE_ON; // LF
  else
    mode &= ~MODE_LOW_FREQ_MODE_ON; // HF
  sx127x_write_reg(self, REG_OP_MODE, mode);
}
//----------------------------------------------------------------------------
// set LNA boost on/off (only for high frequency band)
void sx127x_set_lna_boost(sx127x_t *self, bool lna_boost)
{
  u8_t reg = sx127x_read_reg(self, REG_LNA);

  if (lna_boost)
    reg |= 0x03;  // set `LnaBoostHf` to 3 (boost on, 150% LNA current)
  else
    reg &= ~0x03; // set `LnaBoostHf` to 0 (default LNA current)

  sx127x_write_reg(self, REG_LNA, reg);
}
//----------------------------------------------------------------------------
// get current RX gain code [1..6] from `RegLna` (1 - maximum gain)
u8_t sx127x_get_rx_gain(sx127x_t *self)
{
  return (sx127x_read_reg(self, REG_LNA) >> 5) & 0x07;
}
//----------------------------------------------------------------------------
// get RSSI [dB]
i16_t sx127x_get_rssi(sx127x_t *self)
{
  // FIXME
  if (self->mode == SX127X_LORA) // LoRa mode
    return ((i16_t) sx127x_read_reg(self, REG_PKT_RSSI_VALUE)) -
           (self->freq < 600000000 ? 164 : 157); // F_LF<=525, F_HF>=779 MHz
  else // FSK/OOK mode
    return (- (i16_t) sx127x_read_reg(self, REG_RSSI_VALUE)) >> 1;

}
//----------------------------------------------------------------------------
// get SNR [dB] (LoRa)
i16_t sx127x_get_snr(sx127x_t *self)
{
  if (self->mode == SX127X_LORA) // LoRa mode
  {
    u16_t reg = (u16_t) sx127x_read_reg(self, REG_PKT_SNR_VALUE);
    i16_t snr = (i16_t) reg;
    if (snr & 0x80) // sign bit is 1
      snr -= 256;
    return snr >> 2; // dB
  }
  return 0; // FSK/OOK
}
//----------------------------------------------------------------------------
// set TX power levels, select/deselect PA_BOOST pin
// 1. if PA_BOOST pin selected then:
//    Pout = 2 + out_power = 2...17 dBm
//    (Note: "HighPower" add +3 dBm to PA_BOOST pin)
// 2. if RFO pin selected then:
//    Pmax = 10.8 + 0.6 * max_power = 10.8...15 dBm
//    Pout = Pmax - 15 + out_power  = -4.2...15 dBm
i8_t sx127x_set_power_ex(sx127x_t *self,
                         bool pa_boost,  // `PA_BOOST`    true/false
                         u8_t out_power, // `OutputPower` 0...15 dB
                         u8_t max_power) // `MaxPower`    0...7 (7=default)
{
  i16_t out;
  out_power = SX127X_LIMIT(out_power, 0, 15);
  max_power = SX127X_LIMIT(max_power, 0, 7);

  self->pa_boost = pa_boost;
  if (pa_boost)
  { // select PA_BOOST pin
    sx127x_write_reg(self, REG_PA_CONFIG, out_power | PA_SELECT);
    out = 2 + out_power; // dBm
  }
  else
  { // select RFO pin
    sx127x_write_reg(self, REG_PA_CONFIG, out_power | (max_power << 4));
    out = ((108 - 150 + 5) + 6 * max_power + 10 * out_power) / 10; // dBm
  }

  SX127X_DBG("set Output Power to %i dBm on %s pin",
             out, pa_boost ? "PA_BOOST" : "RFO");

  return (i8_t) out;
}
//----------------------------------------------------------------------------
// set TX output power: -4...+15 dBm on RFO or +2...+17 dBm on PA_BOOST
i8_t sx127x_set_power_dbm(sx127x_t *self, i8_t dBm)
{
  i16_t out;
  
  if (self->pa_boost)
  { // +2...+17 dBm on PA_BOOST pin
    u8_t out_power = (u8_t) SX127X_LIMIT(dBm - 2, 0, 15);
    sx127x_write_reg(self, REG_PA_CONFIG, out_power | PA_SELECT);
    out = 2 + out_power; // dBm
  }
  else if (dBm >= 6)
  { // 0...+15 dBm on RFO pin (`MaxPower`=7)
    u8_t out_power = (u8_t) SX127X_LIMIT(dBm, 0, 15);
    sx127x_write_reg(self, REG_PA_CONFIG, out_power | (7 << 4));
    out = ((108 + 6 * 7 - 150 + 5) + 10 * out_power) / 10; // dBm
  }
  else
  { // -4...+10 dBm on RFO pin (`MaxPower`=0)
    u8_t out_power = (u8_t) SX127X_LIMIT(dBm + 4, 0, 15);
    sx127x_write_reg(self, REG_PA_CONFIG, out_power);
    out = ((108 + 6 * 0 - 150 + 5) + 10 * out_power) / 10; // dBm
  }
  
  SX127X_DBG("set Output Power to %i dBm on %s pin",
             out, self->pa_boost ? "PA_BOOST" : "RFO");
  
  return (i8_t) out;
}
//----------------------------------------------------------------------------
// set high power (+3 dB) on PA_BOOST pin (up to +20 dBm output power)
// (note: you must set OCP trmimmer if set high power, look datasheet)
void sx127x_set_high_power(sx127x_t *self, bool on)
{
  if (on)
    sx127x_write_reg(self, REG_PA_DAC, 0x87); // high power mode
  else
    sx127x_write_reg(self, REG_PA_DAC, 0x84); // default mode
    
  SX127X_DBG("set High Power mode (+3 dB on PA_BOOST pin) to '%s'",
             on ? "On" : "Off");
}
//----------------------------------------------------------------------------
// set trimming of OCP current (45...240 mA)
void sx127x_set_ocp(sx127x_t *self, u8_t trim_mA, bool on)
{
  i16_t trim;

  if (trim_mA <= 120)
    trim = (trim_mA + (2 - 45)) / 5;
  else
    trim = (trim_mA + (5 + 30)) / 10;

  trim = SX127X_LIMIT(trim, 0, 27);
  
  SX127X_DBG("set trimming of OCP current to %i mA (code=%i); OCP is '%s'",
             trim <= 15 ? trim * 5 + 45 : trim * 10 - 30,
             (int) trim,
	     on ? "On" : "Off");

  if (on)
    trim |= 0x20; // `OcpOn`

  sx127x_write_reg(self, REG_OCP, (u8_t) (trim & 0xFF));
}
//----------------------------------------------------------------------------
// set PA rise/fall time of ramp code 0..15 (FSK/LoRa), default is 9
// set modulation shaping code (0..3 in FSK, 0...2 in OOK), default is 0
void sx127x_set_ramp(sx127x_t *self, u8_t shaping, u8_t ramp)
{
  u8_t reg;

  shaping = SX127X_LIMIT(shaping, 0, 3);
  ramp    = SX127X_LIMIT(ramp,    0, 15);
    
  reg = sx127x_read_reg(self, REG_PA_RAMP);
  reg = (reg & 0x90) | (shaping << 5) | ramp;
  sx127x_write_reg(self, REG_PA_RAMP, reg);

  SX127X_DBG("set Shaping=%d and Ramp=%d", (int) shaping, (int) ramp);
}
//----------------------------------------------------------------------------
// enable/disable CRC, set/unset `CrcAutoClearOff` in FSK/OOK mode
void sx127x_enable_crc(sx127x_t *self, bool crc, bool crcAutoClearOff)
{
  self->crc = crc;
  if (self->mode == SX127X_LORA) // LoRa mode
  {
    u8_t reg = sx127x_read_reg(self, REG_MODEM_CONFIG_2);
    reg = crc ? (reg | 0x04) : (reg & ~0x04); // `RxPayloadCrcOn`
    sx127x_write_reg(self, REG_MODEM_CONFIG_2, reg);
    
    SX127X_DBG("set CrcOn=%d (LoRa)", (int) !!crc);
  }
  else // FSK/OOK mode
  {
    u8_t reg = sx127x_read_reg(self, REG_PACKET_CONFIG_1) & ~0x18;
    if (crc)             reg |= 0x10; // `CrcOn`
    if (crcAutoClearOff) reg |= 0x08; // `CrcAutoClearOff`
    sx127x_write_reg(self, REG_PACKET_CONFIG_1, reg);

    SX127X_DBG("set CrcOn=%d; CrcAutoClearOff=%d (FSK/OOK)",
               (int) !!crc, (int) !!crcAutoClearOff);
  }
}
//----------------------------------------------------------------------------
// set PLL bandwidth 0=75, 1=150, 2=225, 3=300 kHz (LoRa/FSK/OOK), default 3
void sx127x_set_pll_bw(sx127x_t *self, u8_t bw)
{
  u8_t reg;
  bw = SX127X_LIMIT(bw, 0, 3);
  reg = sx127x_read_reg(self, REG_PLL);
  reg = (reg & 0x3F) | (bw << 6);
  sx127x_write_reg(self, REG_PLL, reg);
  
  SX127X_DBG("set PLL bandwidth to %d kHz",
             bw == 0 ?  75 :
             bw == 1 ? 150 :
             bw == 2 ? 225 :
                       300);
}
//----------------------------------------------------------------------------
// set signal Bandwidth 7800...500000 Hz (LoRa)
void sx127x_set_bw(sx127x_t *self, u32_t bw)
{
  if (self->mode == SX127X_LORA) // LoRa mode
  {
    u8_t ix;
    sx127x_bw_pack(&bw, &ix);
    u8_t reg = sx127x_read_reg(self, REG_MODEM_CONFIG_1) & 0x0F;
    sx127x_write_reg(self, REG_MODEM_CONFIG_1, reg | (ix << 4));

    SX127X_DBG("set bandwidth (BW) in LoRa mode to %d.%02d kHz (code=%d)",
               (int) bw / 1000, (int) (bw % 1000) / 10, (int) ix);
  }
}
//----------------------------------------------------------------------------
// set Coding Rate 5...8 (LoRa)
void sx127x_set_cr(sx127x_t *self, u8_t cr)
{
  if (self->mode == SX127X_LORA) // LoRa mode
  {
    u8_t reg = sx127x_read_reg(self, REG_MODEM_CONFIG_1);
    cr = SX127X_LIMIT(cr, 5, 8) - 4; // 5...8 -> 1...4
    reg = (reg & 0xF1) | (cr << 1);
    sx127x_write_reg(self, REG_MODEM_CONFIG_1, reg);
    
    SX127X_DBG("set Coding Rate (CR) in LoRa mode to 4/%d (code=%d)",
               cr + 4, cr);
  }
}
//----------------------------------------------------------------------------
// on/off "ImplicitHeaderMode" (LoRa)
// (with SF=6 implicit header mode is the only mode of operation possible)
void sx127x_impl_hdr(sx127x_t *self, bool impl_hdr)
{
  if (self->mode == SX127X_LORA) // LoRa mode
  {
    u8_t reg = sx127x_read_reg(self, REG_MODEM_CONFIG_1);
    reg = (self->impl_hdr = impl_hdr) ? (reg | 0x01) : (reg & 0xFE);
    sx127x_write_reg(self, REG_MODEM_CONFIG_1, reg);
    
    SX127X_DBG("set `ImplicitHeaderMode` in LoRa mode to %d",
               impl_hdr ? 1 : 0);
  }
}
//----------------------------------------------------------------------------
// set Spreading Factor 6...12 (LoRa)
void sx127x_set_sf(sx127x_t *self, u8_t sf)
{
  if (self->mode == SX127X_LORA) // LoRa mode
  {
    u8_t reg = sx127x_read_reg(self, REG_MODEM_CONFIG_2);
    sf = SX127X_LIMIT(sf, 6, 12);
    reg = (reg & 0x0F) | (sf << 4);
    sx127x_write_reg(self, REG_MODEM_CONFIG_2,      reg);
    sx127x_write_reg(self, REG_DETECT_OPTIMIZE,     sf == 6 ? 0xC5 : 0xC3);
    sx127x_write_reg(self, REG_DETECTION_THRESHOLD, sf == 6 ? 0x0C : 0x0A);
    
    SX127X_DBG("set Spreading Factor (SF) in LoRa mode to %d", sf);
  }
}
//----------------------------------------------------------------------------
// set Low Data Rate Optimisation (LoRa)
void sx127x_set_ldro(sx127x_t *self, bool ldro)
{
  if (self->mode == SX127X_LORA) // LoRa mode
  {
    u8_t reg = sx127x_read_reg(self, REG_MODEM_CONFIG_3) & ~0x08;
    if (ldro)
      reg |= 0x08; // `LowDataRateOptimize`
    sx127x_write_reg(self, REG_MODEM_CONFIG_3, reg);
    
    SX127X_DBG("set Low Data Rate Optimisation (LDRO) in LoRa mode to '%s'",
              ldro ? "true" : "false");
  }
}
//----------------------------------------------------------------------------
// set preamble length [6...65535] (LoRa)
void sx127x_set_preamble(sx127x_t *self, u16_t length)
{
  if (self->mode == SX127X_LORA) // LoRa mode
  {
    length = SX127X_LIMIT(length, 6, 65535);
    sx127x_write_reg(self, REG_PREAMBLE_MSB, (u8_t) (length >> 8));
    sx127x_write_reg(self, REG_PREAMBLE_LSB, (u8_t) (length & 0xFF));
    
    SX127X_DBG("set Preamble Length in LoRa mode to %i", (int) length);
  }
}
//----------------------------------------------------------------------------
// set Sync Word (LoRa)
void sx127x_set_sw(sx127x_t *self, u8_t sw)
{
  if (self->mode == SX127X_LORA) // LoRa mode
  {
    sx127x_write_reg(self, REG_SYNC_WORD, sw);
    SX127X_DBG("set Sync Word (SW) in LoRa mode to 0x%02X", (int) sw);
  }
}
//----------------------------------------------------------------------------
// invert IQ channels (LoRa)
void sx127x_invert_iq(sx127x_t *self, bool invert)
{
  if (self->mode == SX127X_LORA) // LoRa mode
  {
    u8_t reg = sx127x_read_reg(self, REG_INVERT_IQ);
    if (invert) reg |=  0x40; // `InvertIq`=1
    else        reg &= ~0x40; // `InvertIq`=0
    sx127x_write_reg(self, REG_INVERT_IQ, reg);

    SX127X_DBG("set `InvertIq` in LoRa mode to '%s'",
               invert ? "true" : "false");
  }
}
//----------------------------------------------------------------------------
// select Continuous mode, must use DIO2->DATA, DIO1->DCLK (FSK/OOK)
void sx127x_continuous(sx127x_t *self, bool on)
{
  if (self->mode != SX127X_LORA) // FSK/OOK mode
  {
    u8_t reg = sx127x_read_reg(self, REG_PACKET_CONFIG_2);
    if (on) reg &= ~0x40; // bit 6: `DataMode` 0 -> Continuous mode
    else    reg |=  0x40; // bit 6: `DataMode` 1 -> Packet mode
    sx127x_write_reg(self, REG_PACKET_CONFIG_2, reg);
    
    SX127X_DBG("set Continuous mode (FSK/OOK) to '%s'", on ? "On" : "Off");
  }
}
//----------------------------------------------------------------------------
// RSSI and IQ callibration (FSK/OOK)
void sx127x_rx_calibrate(sx127x_t *self)
{
  if (self->mode != SX127X_LORA) // FSK/OOK mode
  {
    u8_t reg = sx127x_read_reg(self, REG_IMAGE_CAL);
    reg |= 0x40; // set `ImageCalStart` bit
    sx127x_write_reg(self, REG_IMAGE_CAL, reg);
    
    SX127X_DBG("start RSSI and IQ callibration (FSK/OOK)");

    while (sx127x_read_reg(self, REG_IMAGE_CAL) & 0x20) // `ImageCalRunning`
    {
      // FIXME: check timeout
    }
  }
}
//----------------------------------------------------------------------------
// set bitrate [bit/s] (FSK/OOK)
// (note: must be set between 500 bit/s and 500 kbit/s)
u32_t sx127x_set_bitrate(sx127x_t *self, u32_t bitrate)
{
  if (self->mode != SX127X_LORA) // FSK/OOK mode
  {
    u8_t frac;
    u32_t code;

    if (self->mode == SX127X_FSK)
    { // FSK mode
      code    = ((u32_t) (FREQ_OSC*16) + (bitrate >> 1)) / bitrate;
      bitrate = ((u32_t) (FREQ_OSC*16) + (code >> 1))    / code;
      frac = (u8_t) (code & 0x0F);
      code >>= 4;
    }
    else
    { // OOK mode
      code    = ((u32_t) FREQ_OSC + (bitrate >> 1)) / bitrate;
      bitrate = ((u32_t) FREQ_OSC + (code    >> 1)) / code;
      frac = 0;
    }

    code = SX127X_LIMIT(code, 0x20, 0xFFFF); // 488...1000000 bit/s
    
    sx127x_write_reg(self, REG_BITRATE_MSB, (u8_t) (code >> 8));
    sx127x_write_reg(self, REG_BITRATE_LSB, (u8_t) code);
    sx127x_write_reg(self, REG_BITRATE_FRAC, frac);
  
    SX127X_DBG("set Bitrate in FSK/OOK mode to %d bit/s (code=%i, frac=%i)",
               (int) bitrate, (int) code, (int) frac);

    return bitrate;
  }

  return 0; // return in LoRa mode
}
//----------------------------------------------------------------------------
// set frequency deviation [Hz] (FSK)
// (note: must be set between 600 Hz and 200 kHz)
void sx127x_set_fdev(sx127x_t *self, u32_t fdev)
{
  if (self->mode != SX127X_LORA) // FSK/OOK mode
  {
    u32_t f, f1, f2, f11, f12, f21, f22;
    u8_t fmsb, flsb;

    // FREQ_MAGIC_1 = 8     // arithmetic shift
    // FREQ_MAGIC_2 = 625   // 5**4
    // FREQ_MAGIC_3 = 25    // 5**2
    // FREQ_MAGIC_4 = 15625 // 625*25
    f1  = (fdev / FREQ_MAGIC_2) << FREQ_MAGIC_1;
    f2  = (fdev % FREQ_MAGIC_2) << FREQ_MAGIC_1;

    f11 = f1 / FREQ_MAGIC_3;
    f12 = f1 % FREQ_MAGIC_3;

    f21 = f2 / FREQ_MAGIC_4;
    f22 = f2 % FREQ_MAGIC_4;

    f1  = f11 + f21;
    f2  = (f12 * (FREQ_MAGIC_4 / FREQ_MAGIC_3) + f22 + (FREQ_MAGIC_4 / 2)) /
          FREQ_MAGIC_4;
 
    f = SX127X_LIMIT(f1 + f2, 0, 0x3FFF);
  
    fmsb = (u8_t)(f >> 8);
    flsb = (u8_t) f;

    sx127x_write_reg(self, REG_FDEV_MSB, fmsb);
    sx127x_write_reg(self, REG_FDEV_LSB, flsb);
   
    SX127X_DBG("set Frequency Deviation in FSK mode to %lu Hz "
               "(code=%lu)",
               ((((u32_t) flsb) * FREQ_MAGIC_4 + (1<<7)) >> 8) +
                (((u32_t) fmsb) * FREQ_MAGIC_4),
               f);
  }
}
//----------------------------------------------------------------------------
// set RX bandwidth [Hz] (FSK/OOK)
void sx127x_set_rx_bw(sx127x_t *self, u32_t bw)
{
  if (self->mode != SX127X_LORA) // FSK/OOK mode
  {
    u8_t m, e;
    sx127x_rx_bw_pack(&bw, &m, &e);
    sx127x_write_reg(self, REG_RX_BW, (m << 3) | e);

    SX127X_DBG("set RX bandwidth in FSK/OOK to %d.%02d kHz (mant=%d, exp=%d)",
               (int) bw / 1000, (int) (bw % 1000) / 10, (int) m, (int) e);
  }
}
//----------------------------------------------------------------------------
// set AFC bandwidth [Hz] (FSK/OOK)
void sx127x_set_afc_bw(sx127x_t *self, u32_t bw)
{
  if (self->mode != SX127X_LORA) // FSK/OOK mode
  {
    u8_t m, e;
    sx127x_rx_bw_pack(&bw, &m, &e);
    sx127x_write_reg(self, REG_AFC_BW, (m << 3) | e);

    SX127X_DBG("set AFC bandwidth in FSK/OOK to %d.%02d Hz (mant=%d, exp=%d)",
               (int) bw / 1000, (int) (bw % 1000) / 10, (int) m, (int) e);
  }
}
//----------------------------------------------------------------------------
// enable/disable AFC (FSK/OOK)
void sx127x_set_afc(sx127x_t *self, bool afc)
{
  if (self->mode != SX127X_LORA) // FSK/OOK mode
  {
    u8_t reg = sx127x_read_reg(self, REG_RX_CONFIG);
    if (afc) reg |=  0x10; // bit 4: `AfcAutoOn` -> 1
    else     reg &= ~0x10; // bit 4: `AfcAutoOn` -> 0  
    sx127x_write_reg(self, REG_RX_CONFIG, reg);

    SX127X_DBG("set AFC (FSK/OOK) to '%s'", afc ? "On" : "Off");
  }
}
//----------------------------------------------------------------------------
// set Fixed or Variable packet mode (FSK/OOK)
void sx127x_set_fixed(sx127x_t *self, bool fixed)
{
  if (self->mode != SX127X_LORA) // FSK/OOK mode
  {
    u8_t reg = sx127x_read_reg(self, REG_PACKET_CONFIG_1);
    if (fixed) reg &= ~0x80; // bit 7: PacketFormar -> 0 (fixed size)
    else       reg |=  0x80; // bit 7: PacketFormat -> 1 (variable size)
    sx127x_write_reg(self, REG_PACKET_CONFIG_1, reg);

    SX127X_DBG("set Fixed/Variable packet size (FSK/OOK) to '%s'",
               fixed ? "Fixed" : "Variable");

    self->fixed = fixed;
  }
}
//----------------------------------------------------------------------------
// set DcFree mode: 0=Off, 1=Manchester, 2=Whitening (FSK/OOK)
void sx127x_set_dcfree(sx127x_t *self, u8_t dcfree)
{
  if (self->mode != SX127X_LORA) // FSK/OOK mode
  {
    u8_t reg = sx127x_read_reg(self, REG_PACKET_CONFIG_1);
    reg = (reg & 0x9F) | ((dcfree & 3) << 5); // bits 6-5 `DcFree`
    sx127x_write_reg(self, REG_PACKET_CONFIG_1, reg);
    
    SX127X_DBG("set DC Free mode (FSK/OOK) to '%s'",
               dcfree == 0 ? "Off"        :
               dcfree == 1 ? "Manchester" :
               dcfree == 2 ? "Whitening"  :
                             "Unknown");
  }
}
//----------------------------------------------------------------------------
// on/off fast frequency PLL hopping (FSK/OOK)
void sx127x_set_fast_hop(sx127x_t *self, bool on)
{
  if (self->mode != SX127X_LORA) // FSK/OOK mode
  {
    u8_t reg = sx127x_read_reg(self, REG_PLL_HOP);
    reg = on ? (reg | 0x80) : (reg & 0x7F); // `FastHopOn`
    sx127x_write_reg(self, REG_PLL_HOP, reg);
    
    SX127X_DBG("set Fast Frequency PLL hopping (FSK/OOK) to '%s'",
               on ? "On" : "Off");
  }
}
//----------------------------------------------------------------------------
// send packet (LoRa/FSK/OOK)
// fixed - implicit header mode (LoRa), fixed packet length (FSK/OOK)
i16_t sx127x_send(sx127x_t *self, const u8_t *data, i16_t size, bool fixed)
{
  int i;
  sx127x_standby(self);
  u32_t cnt;

  // check size
  if (size <= 0) return SX127X_ERR_BAD_SIZE;
  size = SX127X_MIN(size, MAX_PKT_LENGTH);

  if (self->mode == SX127X_LORA) // LoRa mode
  {
    // set implicit or explicit header mode
    if (self->impl_hdr != fixed)
      sx127x_impl_hdr(self, fixed);

    // set FIFO base address
    sx127x_write_reg(self, REG_FIFO_ADDR_PTR, FIFO_TX_BASE_ADDR);

    // write data to FIFO
    for (i = 0; i < size; i++)
      sx127x_write_reg(self, REG_FIFO, data[i]);

    // set payload length
    sx127x_write_reg(self, REG_PAYLOAD_LENGTH, (u8_t) size);

    // start TX packet
    sx127x_tx(self);

    // wait for TX done, standby automatically on TX_DONE
    while ((sx127x_read_reg(self, REG_IRQ_FLAGS) & IRQ_TX_DONE) == 0)
    {
      // FIXME: check timeout
    }

    // clear IRQ's
    sx127x_write_reg(self, REG_IRQ_FLAGS, IRQ_TX_DONE);
  }
  else // FSK/OOK mode
  {
    //u8_t add;
    
    // set fixed or variable packet length
    if (self->fixed != fixed)
      sx127x_set_fixed(self, fixed);

    // set TX start FIFO condition
    //sx127x_write_reg(self, REG_FIFO_THRESH, TX_START_FIFO_NOEMPTY);
    
#if 0
    SX127X_DBG("#0 RegIrqFlags1=0x%02X",
	       sx127x_read_reg(self, REG_IRQ_FLAGS_1));
    SX127X_DBG("#0 RegIrqFlags2=0x%02X",
	       sx127x_read_reg(self, REG_IRQ_FLAGS_2));
#endif
    
    // wait while FIFO is no empty
    cnt = 1000000000; // FIXME: callibrate timeout
    while ((sx127x_read_reg(self, REG_IRQ_FLAGS_2) & IRQ2_FIFO_EMPTY) == 0)
    {
      // FIXME: check timeout
#if 0
      SX127X_DBG("#1 RegIrqFlags1=0x%02X",
                 sx127x_read_reg(self, REG_IRQ_FLAGS_1));
      SX127X_DBG("#1 RegIrqFlags2=0x%02X",
                 sx127x_read_reg(self, REG_IRQ_FLAGS_2));
#endif
      if (--cnt == 0)
      {
        SX127X_DBG("stop waiting `FifoEmpty` by timeout");
        break; // exit by timeout
      }
    }

    if (self->fixed)
    { // fixed packet length
      sx127x_write_reg(self, REG_PAYLOAD_LEN, size);
      //add = 0;
    }
    else
    { // variable packet length
      sx127x_write_reg(self, REG_FIFO, size);
      //add = 1;
    }
    
    // set TX start FIFO condition
    //sx127x_write_reg(self, REG_FIFO_THRESH, TX_START_FIFO_LEVEL | (size + add));
    
    // write data to FIFO
    for (i = 0; i < size; i++)
      sx127x_write_reg(self, REG_FIFO, data[i]);
    
    // start TX packet
    sx127x_tx(self);
   
    // wait `TxRaedy` (bit 5 in `RegIrqFlags1`)
    //while ((sx127x_read_reg(self, REG_IRQ_FLAGS_1) & IRQ1_TX_READY) == 0)
    //{
    //  // FIXME: check timeout
    //}

    // wait `PacketSent` (bit 3 in `RegIrqFlags2`)
    cnt = 1000000000; // FIXME: callibrate timeout
    while ((sx127x_read_reg(self, REG_IRQ_FLAGS_2) & IRQ2_PACKET_SENT) == 0)
    {
      // FIXME: check timeout
#if 0
      SX127X_DBG("#2 RegIrqFlags1=0x%02X",
                 sx127x_read_reg(self, REG_IRQ_FLAGS_1));
      SX127X_DBG("#2 RegIrqFlags2=0x%02X",
                 sx127x_read_reg(self, REG_IRQ_FLAGS_2));
#endif
      if (--cnt == 0)
      {
        SX127X_DBG("stop waiting `PacketSent` by timeout");
        break; // exit by timeout
      }
    }
    
    // switch to standby mode
    sx127x_standby(self);
    sx127x_standby(self); // FIXME: why second?
  }

  return SX127X_ERR_NONE;
}
//----------------------------------------------------------------------------
// go to RX mode; wait callback by interrupt (LoRa/FSK/OOK)
// LoRa:    if pkt_len = 0 then explicit header mode, else - implicit
// FSK/OOK: if pkt_len = 0 then variable packet length, else - fixed
void sx127x_receive(sx127x_t *self, i16_t pkt_len)
{
  pkt_len = SX127X_MIN(pkt_len, MAX_PKT_LENGTH);

  if (self->mode == SX127X_LORA) // LoRa mode
  {
    if (pkt_len > 0)
    { // implicit header mode
      if (!self->impl_hdr) sx127x_impl_hdr(self, true);
      sx127x_write_reg(self, REG_PAYLOAD_LENGTH, (u8_t) pkt_len);
    }
    else
    { // explicit header mode
      if (self->impl_hdr) sx127x_impl_hdr(self, false);
    }
  }
  else // FSK/OOK mode
  {
    if (pkt_len > 0)
    { // fixed packet length
      if (!self->fixed) sx127x_set_fixed(self, true);
      sx127x_write_reg(self, REG_PAYLOAD_LEN, (u8_t) pkt_len);
    }
    else
    { // variable packet length
      if (self->fixed) sx127x_set_fixed(self, false);
      sx127x_write_reg(self, REG_PAYLOAD_LEN, MAX_PKT_LENGTH);
    }
  }

  sx127x_rx(self);
}
//----------------------------------------------------------------------------
// enable/disable interrupt by RX done for debug (LoRa)
void sx127x_enable_rx_irq(sx127x_t *self, bool enable)
{
  if (self->mode == SX127X_LORA) // LoRa mode
  {
    u8_t reg = sx127x_read_reg(self, REG_IRQ_FLAGS_MASK);
    if (enable) reg &= ~IRQ_RX_DONE_MASK;
    else        reg |=  IRQ_RX_DONE_MASK;
    sx127x_write_reg(self, REG_IRQ_FLAGS_MASK, reg);
  }
}
//----------------------------------------------------------------------------
// get IRQ flags for debug
u16_t sx127x_get_irq_flags(sx127x_t *self)
{
  if (self->mode == SX127X_LORA) // LoRa mode
  {
    u8_t irq_flags = sx127x_read_reg(self, REG_IRQ_FLAGS);
    sx127x_write_reg(self, REG_IRQ_FLAGS, irq_flags);
    return (u16_t) irq_flags;
  }
  else // FSK/OOK mode
  {
    u16_t irq_flags_1 = (u16_t) sx127x_read_reg(self, REG_IRQ_FLAGS_1); 
    u16_t irq_flags_2 = (u16_t) sx127x_read_reg(self, REG_IRQ_FLAGS_2); 
    return (irq_flags_2 << 8) | irq_flags_1;
  }
}
//----------------------------------------------------------------------------
// IRQ handler on DIO0 pin
void sx127x_irq_handler(sx127x_t *self)
{
  bool crc_ok;
  i16_t payload_len;
  int i;
    
  //SX127X_DBG("start sx127x_irq_handler()"); // FIXME

  if (self->mode == SX127X_LORA) // LoRa mode
  {
    u8_t irq_flags = sx127x_read_reg(self, REG_IRQ_FLAGS); // should be 0x50
    sx127x_write_reg(self, REG_IRQ_FLAGS, irq_flags);

    if ((irq_flags & IRQ_RX_DONE) == 0) // check `RxDone`
    {
#if 0
      SX127X_DBG("IRQ on DIO0 (LoRa): RegIrqFlags=0x%02X",
                 irq_flags);
#endif
      return; // `RxDone` is not set
    }

    SX127X_DBG("IRQ on DIO0 (LoRa) by `RxDone`: RegIrqFlags=0x%02X",
       	       irq_flags);

    // check `PayloadCrcError` bit
    crc_ok = !(irq_flags & IRQ_PAYLOAD_CRC_ERROR);

    // set FIFO address to current RX address
    sx127x_write_reg(self, REG_FIFO_ADDR_PTR,
                     sx127x_read_reg(self, REG_FIFO_RX_CURRENT_ADDR));

    // read payload length
    payload_len = self->impl_hdr ?
                  sx127x_read_reg(self, REG_PAYLOAD_LENGTH) :
                  sx127x_read_reg(self, REG_RX_NB_BYTES);
  }
  else // FSK/OOK mode
  {
    u8_t irq_flags2 = sx127x_read_reg(self, REG_IRQ_FLAGS_2); // ~ 0x26/0x24
    
    if ((irq_flags2 & IRQ2_PAYLOAD_READY) == 0) // check `PayloadReady`
    {
#if 0
      u8_t irq_flags1 = sx127x_read_reg(self, REG_IRQ_FLAGS_1);
      SX127X_DBG("IRQ on DIO0 (FSK/OOK): "
                 "RegIrqFlags1=0x%02X, "
                 "RegIrqFlags2=0x%02X",
                  irq_flags1, irq_flags2);
#endif
      return; // `PayloadReady` is not set in RegIrqFlags2
    }
  
    SX127X_DBG("IRQ on DIO0 (FSK/OOK) by `PayloadReady`: "
               "RegIrqFlags2=0x%02X", irq_flags2);
  
    // check `CrcOk` bit
    crc_ok = !!(irq_flags2 & IRQ2_CRC_OK);

    // read payload length
    if (sx127x_read_reg(self, REG_PACKET_CONFIG_1) & 0x80) // `PacketFormat`
      payload_len = sx127x_read_reg(self, REG_FIFO); // variable length
    else
      payload_len = sx127x_read_reg(self, REG_PAYLOAD_LEN); // fixed length
  }
  
  // read FIFO
  for (i = 0; i < payload_len; i++)
    self->payload[i] = sx127x_read_reg(self, REG_FIFO);

  // run callback
  if (self->on_receive != (void (*)(sx127x_t*, u8_t*, u8_t, bool, void*)) NULL)
    self->on_receive(self, self->payload, payload_len,
                     self->crc ? crc_ok : true,
                     self->on_receive_context);
}
//----------------------------------------------------------------------------
// dump registers for debug
void sx127x_dump(sx127x_t *self)
{
  int i;
  for (i = 0; i < 128; i++)
    SX127X_DBG("Reg[0x%02X] = 0x%02X", i, (int) sx127x_read_reg(self, i));
}
//----------------------------------------------------------------------------

/*** end of "sx127x.c" file ***/


