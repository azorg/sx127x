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
//-----------------------------------------------------------------------------
int sx127x_init(sx127x_t *self)
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
void sx127x_irq_handler(sx127x_t *self)
{
  printf(">>> run sx127x_irq_handler()\n");
  //...
}
//----------------------------------------------------------------------------

/*** end of "sx127x.c" file ***/


