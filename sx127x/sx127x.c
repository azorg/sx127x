/*
 * -*- coding: UTF8 -*-
 * Semtech SX127x famaly chips driver
 * File: "sx127x.c"
 */

//-----------------------------------------------------------------------------
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
  false,     // CRC in packet modes false - off, true - on

  // LoRaTM mode:
  125000, // Bandwith [Hz]: 7800...500000 (125000 -> 125 kHz)
  9,      // Spreading Facror: 6..12
  5,      // Code Rate: 5...8
  -1,     // Low Data Rate Optimize: 1 - on, 0 - off, -1 - automatic
  0x12,   // Sync Word (allways 0x12)
  8,      // Size of preamble: 6...65535 (8 by default)
  false,  // Implicit Header on/off
 
  // FSK/OOK mode:
  4800,  // bitrate [bit/s] (4800 bit/s for example)
  5000,  // frequency deviation [Hz] (5000 Hz for example)
  10400, // RX  bandwidth [Hz]: 2600...250000 kHz (10400 -> 10.4 kHz)
  2600,  // AFC bandwidth [Hz]: 2600...250000 kHz (2600  ->  2.6 kHz)
  false, // AFC on/off
  false, // false - variable packet size, true - fixed packet size
  0      // DC free method: 0 - None, 1 - Manchester, 2 - Whitening
};
//-----------------------------------------------------------------------------
// BandWith table [kHz] (LoRa)
u32_t sx127x_bw_table[] = BW_TABLE; // Hz

// RX BandWith table (FSK/OOK)
typedef struct sx127x_rf_bw_tbl_ {
   u8_t mant; // 0...2
   u8_t exp;  // 1...7
  u32_t bw;   // 2600...250000 Hz
} sx127x_rf_bw_tbl_t;

sx127x_rf_bw_tbl_t sx127x_rf_bw_tbl[] = RX_BW_TABLE;
//-----------------------------------------------------------------------------
// pack RX bandwidth in Hz to mant/exp micro float format
void sx127x_rx_bw_pack(u32_t bw, u8_t *mant, u8_t *exp)
{
  int i, n = sizeof(sx127x_rf_bw_tbl) / sizeof(sx127x_rf_bw_tbl_t);

  for (i = 0; i < n; i++)
  {
    if (bw <= sx127x_rf_bw_tbl[i].bw)
    {
      *mant = sx127x_rf_bw_tbl[i].mant;
      *exp  = sx127x_rf_bw_tbl[i].exp;
      return;
    }
  }

  *mant = sx127x_rf_bw_tbl[n - 1].mant;
  *exp  = sx127x_rf_bw_tbl[n - 1].exp;
}
//----------------------------------------------------------------------------
// SPI transfer help function
static u8_t sx127x_spi_transfer(sx127x_t *self, u8_t address, u8_t value)
{
  u8_t tx_buf[2], rx_buf[2];
  
  // prepare TX buffer
  tx_buf[0] = address;
  tx_buf[1] = value;

  // transfe data over SPI
  self->spi_cs(true); // select crystall
  self->spi_exchange(tx_buf, rx_buf, 2, self->spi_exchange_context);
  self->spi_cs(false); // unselect crystall
  
  return rx_buf[1];
}
//-----------------------------------------------------------------------------
// read SX127x 8-bit register by SPI help function
u8_t sx127x_read_reg(sx127x_t *self, u8_t address)
{
  return sx127x_spi_transfer(self, address & 0x7F, 0);
}
//-----------------------------------------------------------------------------
// read SX127x 8-bit register by SPI help function
void sx127x_write_reg(sx127x_t *self, u8_t address, u8_t value)
{
  sx127x_spi_transfer(self, address | 0x80, value);
}
//-----------------------------------------------------------------------------
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
  void *on_receive_context)   // optional on_receive() context
{
  if (mode != SX127X_FSK && mode != SX127X_OOK) mode = SX127X_LORA;

  self->mode         = mode;
  self->spi_cs       = spi_cs;
  self->spi_exchange = spi_exchange;
  self->on_receive   = on_receive;

  self->spi_exchange_context = spi_exchange_context;
  self->on_receive_context   = on_receive_context;

  return sx127x_set_pars(self, pars);
}
//----------------------------------------------------------------------------
// setup SX127x radio module (uses from sx127x_init())
int sx127x_set_pars(
  sx127x_t *self,
  const sx127x_pars_t *pars) // configuration parameters or NULL
{
  u8_t version;
  
  if (pars == (sx127x_pars_t*) NULL)
    self->pars = sx127x_pars_default;
  else
    self->pars = *pars;

  // chek version
  version = sx127x_version(self);
  SX127X_DBG("SX127x selicon revision = 0x%02X", version);
  if (version != 0x12)
    return SX127X_ERR_VERSION;

  // switch mode
  if      (self->mode == SX127X_FSK) sx127x_fsk(self);  // FSK
  else if (self->mode == SX127X_OOK) sx127x_ook(self);  // OOK
  else                               sx127x_lora(self); // LoRa


  return SX127X_ERR_NONE;
}
//----------------------------------------------------------------------------
// free SX127x radio module
void sx127x_free(sx127x_t *self)
{

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
// switch SX127x radio module to LoRaTM mode
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
bool sx12x_is_lora(sx127x_t *self)
{
  u8_t mode = sx127x_read_reg(self, REG_OP_MODE); // read mode
  return mode & MODE_LONG_RANGE ? true : false;
}
//----------------------------------------------------------------------------
// switch SX127x radio module to FSK mode
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
// switch SX127x radio module to OOK mode
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
//...
//----------------------------------------------------------------------------
// IRQ handler on DIO0 pin
void sx127x_irq_handler(sx127x_t *self)
{
  printf(">>> start sx127x_irq_handler()\n");
  //...
}
//----------------------------------------------------------------------------

/*** end of "sx127x.c" file ***/


