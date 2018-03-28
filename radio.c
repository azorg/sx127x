/*
 * -*- coding: UTF8 -*-
 * Semtech SX127x famaly chips example layer for SBC (Single Board Computer)
 * Tested on "Orange Pi Zero"
 * File: "radio.c"
 */

//----------------------------------------------------------------------------
#include "radio.h"
#include "spi.h"      // `spi_t`
#include "sgpio.h"    // `sgpio_t`
#include "stimer.h"   // stimer_sleep_ms()
#include "vsthread.h" // `vsthread.h`
#include <stdio.h>    // printf()
#include <stdlib.h>   // exit(), EXIT_SUCCESS, EXIT_FAILURE
//----------------------------------------------------------------------------
sx127x_t radio;
int radio_stop = 0;
//----------------------------------------------------------------------------
static spi_t spi;
static vsthread_t thread_irq;

#ifdef RADIO_GPIO_IRQ
static sgpio_t gpio_irq;   // in IRQ
#endif

#ifdef RADIO_GPIO_RESET
static sgpio_t gpio_reset; // out
#endif

#ifdef RADIO_GPIO_CS
static sgpio_t gpio_cs;    // out
#endif

#ifdef RADIO_GPIO_DATA
static sgpio_t gpio_data;  // out
#endif

#ifdef RADIO_GPIO_LED
static sgpio_t gpio_led;   // out
#endif
//-----------------------------------------------------------------------------
// on/off DATA line
void radio_data_on(bool on)
{
#ifdef RADIO_GPIO_DATA
  sgpio_set(&gpio_data, on ? 1 : 0);
#endif
}
//-----------------------------------------------------------------------------
// on/off LED
void radio_led_on(bool on)
{
#ifdef RADIO_GPIO_LED
  sgpio_set(&gpio_led, on ? 1 : 0);
  //sgpio_set(&gpio_data, on ? 1 : 0);
#endif
}
//-----------------------------------------------------------------------------
// blink LED
void radio_blink_led()
{
#ifdef RADIO_GPIO_LED
  radio_led_on(true);
  stimer_sleep_ms(100.);
  radio_led_on(false);
  stimer_sleep_ms(20.);
#else
  printf("RADIO: radio_blink_led()\n");
#endif
}
//-----------------------------------------------------------------------------
// hard reset SX127x radio module
void radio_reset()
{
  printf("RADIO: radio_reset()\n");

#ifdef RADIO_GPIO_RESET
  sgpio_set(&gpio_reset, 1);
  stimer_sleep_ms(100.);
  sgpio_set(&gpio_reset, 0);
  stimer_sleep_ms(100.);
  sgpio_set(&gpio_reset, 1);
  stimer_sleep_ms(100.);
#endif
}
//----------------------------------------------------------------------------
// IRQ waiting thread
static void *thread_irq_fn(void *arg)
{
  printf("RADIO: start irq_thread()\n");
  
#ifdef RADIO_GPIO_IRQ
  while (1)
  {
    // wait interrupt
    int retv  = sgpio_poll(&gpio_irq, 1000); // FIXME
    int value = sgpio_get( &gpio_irq);
    //printf("RADIO: sgpio_poll(&gpio_irq, 1000) return %d\n", retv); // FIXME

    if (retv > 0)
    { // interrupt
      //printf("RADIO: sgpio_get(&gpio_irq) return %d\n", value); // FIXME
      if (value) sx127x_irq_handler(&radio);
    }
    else if (retv == 0)
    { // timeout
      if (radio_stop)
      {
        printf("RADIO: thread_irq_fn() finished by `radio_stop`\n");
        break; // finish by `global_stop`
      }
    }
    else // retv < 0
    { // error
      printf("RADIO: thread_irq_fn() finished by sgpio_poll() error\n");
      radio_stop = 1;
      break; // finish by sgpio_poll() error
    }
  } // while(1)
#endif // RADIO_GPIO_IRQ

  return NULL;
}
//-----------------------------------------------------------------------------
// crystall select for SX127X chip
static void radio_spi_cs(bool cs)
{
#ifdef RADIO_GPIO_CS
  sgpio_set(&gpio_cs, cs ? 0 : 1);
#endif
}
//-----------------------------------------------------------------------------
// init SX127x radio module 
void radio_init()
{
  int retv;

  // setup SPI
  retv = spi_init(&spi,
                  RADIO_SPI_DEVICE, // filename like "/dev/spidev0.0"
                  0,                // SPI_* (look "linux/spi/spidev.h")
                  0,                // bits per word (usually 8)
                  RADIO_SPI_SPEED); // max speed [Hz]
  printf("RADIO: spi_init(device='%s', speed=%d) return %d\n",
         RADIO_SPI_DEVICE, RADIO_SPI_SPEED, retv);
  if (retv != 0) exit(EXIT_FAILURE);

  // setup GPIOs
  if (1)
  {
#ifdef RADIO_GPIO_IRQ
    sgpio_unexport(RADIO_GPIO_IRQ); // FIXME: it is important! Why?
    sgpio_export(RADIO_GPIO_IRQ);
#endif

#ifdef RADIO_GPIO_RESET
    sgpio_export(RADIO_GPIO_RESET);
#endif

#ifdef RADIO_GPIO_CS
    sgpio_export(RADIO_GPIO_CS);
#endif

#ifdef RADIO_GPIO_DATA
    sgpio_export(RADIO_GPIO_DATA);
#endif

#ifdef RADIO_GPIO_LED
    sgpio_export(RADIO_GPIO_LED);
#endif
  }

#ifdef RADIO_GPIO_IRQ
  sgpio_init(&gpio_irq,   RADIO_GPIO_IRQ);
  sgpio_mode(&gpio_irq,   SGPIO_DIR_IN,  SGPIO_EDGE_RISING);

  retv = sgpio_get(&gpio_irq);
  printf("RADIO: first sgpio_get(&gpio_irq) return %d\n", retv); // FIXME
#endif
  
#ifdef RADIO_GPIO_RESET
  sgpio_init(&gpio_reset, RADIO_GPIO_RESET);
  sgpio_mode(&gpio_reset, SGPIO_DIR_OUT, SGPIO_EDGE_NONE);
#endif

#ifdef RADIO_GPIO_CS
  sgpio_init(&gpio_cs,    RADIO_GPIO_CS);
  sgpio_mode(&gpio_cs,    SGPIO_DIR_OUT, SGPIO_EDGE_NONE);
  radio_spi_cs(false);
#endif

#ifdef RADIO_GPIO_DATA
  sgpio_init(&gpio_data,  RADIO_GPIO_DATA);
  sgpio_mode(&gpio_data,  SGPIO_DIR_OUT, SGPIO_EDGE_NONE);
  radio_data_on(false);
#endif
  
#ifdef RADIO_GPIO_DATA
  sgpio_init(&gpio_led,   RADIO_GPIO_LED);
  sgpio_mode(&gpio_led,   SGPIO_DIR_OUT, SGPIO_EDGE_NONE);
  radio_led_on(false);
#endif
  
  // hard reset SX127x radio module
  radio_reset();
  
  // creade listen IRQ threads
  vsthread_create(32, SCHED_FIFO, &thread_irq, thread_irq_fn, NULL);
}
//-----------------------------------------------------------------------------
// free SX127x radio module
void radio_free()
{
  sx127x_free(&radio);

  radio_stop = 1;
  
  // free SPI
  spi_free(&spi);

  // unexport GPIOs
#ifdef RADIO_GPIO_LED
  sgpio_free(&gpio_led);
#endif

#ifdef RADIO_GPIO_DATA
  sgpio_free(&gpio_data);
#endif

#ifdef RADIO_GPIO_CS
  sgpio_free(&gpio_cs);
#endif

#ifdef RADIO_GPIO_RESET
  sgpio_free(&gpio_reset);
#endif

#ifdef RADIO_GPIO_IRQ
  sgpio_free(&gpio_irq);
#endif

  if (1)
  {
#ifdef RADIO_GPIO_LED
    sgpio_unexport(RADIO_GPIO_LED);
#endif

#ifdef RADIO_GPIO_DATA
    sgpio_unexport(RADIO_GPIO_DATA);
#endif

#ifdef RADIO_GPIO_CS
    sgpio_unexport(RADIO_GPIO_CS);
#endif

#ifdef RADIO_GPIO_RESET
    sgpio_unexport(RADIO_GPIO_RESET);
#endif

#ifdef RADIO_GPIO_IRQ
    sgpio_unexport(RADIO_GPIO_IRQ);
#endif
  }
  
  // join IRQ thread
  vsthread_join(thread_irq, NULL);
}
//-----------------------------------------------------------------------------
// SPI exchange wrapper function (return number or RX bytes)
int radio_spi_exchange(
  u8_t       *rx_buf, // RX buffer
  const u8_t *tx_buf, // TX buffer
  u8_t len,           // number of bytes
  void *context)      // optional SPI context or NULL
{
  int retv;

  radio_spi_cs(true);  // select crystall
  retv = spi_exchange(&spi, (char*) rx_buf, (const char*) tx_buf, (int) len);
  radio_spi_cs(false); // unselect crystall

#if 0
  // FIXME SPI debug
  printf("RADIO: spi_exchange([0x%02X, 0x%02X], len=%d) "
         "return ([0x%02X, 0x%02X], retv=%d)\n",
         tx_buf[0], tx_buf[1], (int) len,
         rx_buf[0], rx_buf[1], (int) retv);
#endif

  return retv;
}
//----------------------------------------------------------------------------

/*** end of "radio.c" file ***/


