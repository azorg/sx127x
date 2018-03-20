/*
 * -*- coding: UTF8 -*-
 * Semtech SX127x famaly chips driver
 * File: "sx127x.h"
 */

#ifndef SX127X_H
#define SX127X_H
//-----------------------------------------------------------------------------
// RADIO class mode
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
// `spi_t` type structure
typedef struct sx127x_ {
  int   i; //!!!
} sx127x_t;
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
//----------------------------------------------------------------------------
int sx127x_init(sx127x_t *self,
                const char *device, // filename like "/dev/spidev0.0"
                int mode,           // SPI_* (look "linux/spi/spidev.h")
                int speed);         // max speed [Hz]
//----------------------------------------------------------------------------
void sx127x_free(sx127x_t *self);
//----------------------------------------------------------------------------
//...
//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif // __cplusplus
//----------------------------------------------------------------------------
#endif // SX127X_H

/*** end of "sx127x.h" file ***/


