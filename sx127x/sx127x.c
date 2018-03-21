/*
 * -*- coding: UTF8 -*-
 * Semtech SX127x famaly chips driver
 * File: "sx127x.c"
 */

//-----------------------------------------------------------------------------
#include "sx127x.h"     // `sx127x_t`
#include "sx127x_def.h" // #define's
//-----------------------------------------------------------------------------
// BandWith table [kHz] (LoRa)
float sx127x_bw_table[] = BW_TABLE;

// RX BandWith table (FSK/OOK)
struct {
  unsigned char mant;
  unsigned char exp;
  float kHz;
} sx127x_rf_bw_table[] = RX_BW_TABLE;
//-----------------------------------------------------------------------------
int sx127x_rx_bw_index(float bw)
{
    for m, e, v in RX_BW_TABLE:
        if bw <= v:
            return m, e 
    return RX_BW_TABLE[-1][:2]
{
//-----------------------------------------------------------------------------
int sx127x_init(sx127x_t *self,
                const char *device, // filename like "/dev/spidev0.0"
                int mode,           // SPI_* (look "linux/spi/spidev.h")
                int speed)          // max speed [Hz]
{
  return SX127X_ERR_NONE;
}
//----------------------------------------------------------------------------
void sx127x_free(sx127x_t *self)
{

}
//----------------------------------------------------------------------------
//...
//----------------------------------------------------------------------------

/*** end of "sx127x.c" file ***/


