/*
 * -*- coding: UTF8 -*-
 * Semtech SX127x famaly chips driver (test/example module)
 * File: "sx127x_test.h"
 */

//-----------------------------------------------------------------------------
#include <stdlib.h>     // exit(), EXIT_SUCCESS, EXIT_FAILURE
#include "spi.h"        // `spi_t`
#include "sgpio.h"      // `sgpio_t`
#include "stimer.h"     // `stimer_t`
#include "vsthread.h"   // `vsthread.h`
#include "sx127x.h"     // `sx127x_t`
#include "sx127x_def.h" // SX127x define's
//-----------------------------------------------------------------------------
#define ORANGE_PI_ZERO
//#define ORANGE_PI_ONE
//#define ORANGE_PI_WIN_PLUS

// GPIO lines and SPI devic to SX127x module
#if defined(ORANGE_PI_ZERO)
#  define SPI_DEVICE "/dev/spidev1.0"
#  define GPIO_RESET 7
#  define GPIO_IRQ   6
#  define GPIO_CS    18
#  define GPIO_DATA  19
#  define GPIO_LED   13
#elif defined(ORANGE_PI_ONE)
#  define SPI_DEVICE "/dev/spidev1.0" // FIXME
#  define GPIO_RESET 1 // FIXME
#  define GPIO_IRQ   2 // FIXME
#  define GPIO_CS    3 // FIXME
#  define GPIO_DATA  4 // FIXME
#  define GPIO_LED   5 // FIXME
#elif defined(ORANGE_PI_WIN_PLUS)
#  define SPI_DEVICE "/dev/spidev1.0" // FIXME
#  define GPIO_RESET 1 // FIXME
#  define GPIO_IRQ   2 // FIXME
#  define GPIO_CS    3 // FIXME
#  define GPIO_DATA  4 // FIXME
#  define GPIO_LED   5 // FIXME
#endif

// SPI max speed [Hz]
#define SPI_SPEED 500000 // 5 MHz
//-----------------------------------------------------------------------------
// global variables
spi_t    spi;
sgpio_t  gpio_irq;   // in IRQ
sgpio_t  gpio_reset; // out
sgpio_t  gpio_cs;    // out
sgpio_t  gpio_data;  // out
sgpio_t  gpio_led;   // out
double interval = 1000.; // timer interval [ms]
stimer_t timer;
vsthread_t thread_timer;
vsthread_t thread_irq;
sx127x_t radio;
//-----------------------------------------------------------------------------
// SIGINT handler (Ctrl-C)
static void sigint_handler(void *context)
{
  stimer_stop(&timer);
  fprintf(stderr, "\nCtrl-C pressed\n");
} 
//-----------------------------------------------------------------------------
// blink LED
void blink_led()
{
  printf(">>> blink_led()\n"); // FIXME
  sgpio_set(&gpio_led, 0);
  stimer_sleep_ms(100.);
  sgpio_set(&gpio_led, 1);
  stimer_sleep_ms(20.);
}
//-----------------------------------------------------------------------------
// hard reset SX127x radio module
void reset_radio()
{
  printf(">>> reset_radio()\n");
  sgpio_set(&gpio_reset, 1);
  stimer_sleep_ms(100.);
  sgpio_set(&gpio_reset, 0);
  stimer_sleep_ms(100.);
  sgpio_set(&gpio_reset, 1);
  stimer_sleep_ms(100.);
}
//-----------------------------------------------------------------------------
// periodic timer handler (main periodic function)
static int timer_handler(void *context)
{
  u8_t version;
  printf(">>> start timer_handrer()\n");
  //...
  
  //FIXME !!!
  version = sx127x_version(&radio);
  printf(">>> SX127x selicon revision = 0x%02X\n", version);
  reset_radio();
  blink_led();

  return 0;
}
//-----------------------------------------------------------------------------
// IRQ waiting thread
static void *thread_irq_fn(void *arg)
{
  printf(">>> start irq_thread()\n");
  //!!!pause(); //FIXME This statement only for debug!!!
  while (1)
  {
    // wait interrupt
    int retv = sgpio_poll(&gpio_irq, 1000); //!!! FIXME
    if (retv > 0)
      sx127x_irq_handler(&radio);
  }
  return NULL;
}
//-----------------------------------------------------------------------------
// SPI crystal select by GPIO wrapper function
void on_spi_cs(bool cs)
{
  sgpio_set(&gpio_cs, cs ? 0 : 1); // negative CS
}
//-----------------------------------------------------------------------------
// SPI exchange wrapper function (return number or RX bytes)
int on_spi_exchange(
  u8_t       *rx_buf, // RX buffer
  const u8_t *tx_buf, // TX buffer
  u8_t len,           // number of bytes
  void *context)      // optional SPI context or NULL
{
  spi_t *spi = (spi_t*) context; 
  return spi_exchange(spi, (char*) rx_buf, (const char*) tx_buf, (int) len);
}
//-----------------------------------------------------------------------------
// receive callback
void on_receive(
    sx127x_t *self,     // pointer to sx127x_t object
    u8_t *payload,      // payload data
    u8_t payload_size,  // payload size
    bool crc,           // CRC ok/false
    void *context)      // optional context
{
  fprintf(stderr, ">>> start on_receive()\n");
  blink_led();
  //... FIXME

}
//-----------------------------------------------------------------------------
// simple example usage of `sx127x_t` component 
int main()
{
  int retv;
 
  // setup GPIOs
  if (1)
  {
    sgpio_export(GPIO_IRQ);
    sgpio_export(GPIO_RESET);
    sgpio_export(GPIO_CS);
    sgpio_export(GPIO_DATA);
    sgpio_export(GPIO_LED);
  }

  sgpio_init(&gpio_irq,   GPIO_IRQ);
  sgpio_init(&gpio_reset, GPIO_RESET);
  sgpio_init(&gpio_cs,    GPIO_CS);
  sgpio_init(&gpio_data,  GPIO_DATA);
  sgpio_init(&gpio_led,   GPIO_LED);

  sgpio_mode(&gpio_irq,   SGPIO_DIR_IN,  SGPIO_EDGE_RISING);
  sgpio_mode(&gpio_reset, SGPIO_DIR_OUT, SGPIO_EDGE_NONE);
  sgpio_mode(&gpio_cs,    SGPIO_DIR_OUT, SGPIO_EDGE_NONE);
  sgpio_mode(&gpio_data,  SGPIO_DIR_OUT, SGPIO_EDGE_NONE);
  sgpio_mode(&gpio_led,   SGPIO_DIR_OUT, SGPIO_EDGE_NONE);

  // setup SPI
  retv = spi_init(&spi,
                  SPI_DEVICE, // filename like "/dev/spidev0.0"
                  0,          // SPI_* (look "linux/spi/spidev.h")
                  0,          // bits per word (usually 8)
                  SPI_SPEED); // max speed [Hz]
  printf(">>> spi_init(device='%s', speed=%d) return %d\n",
         SPI_DEVICE, SPI_SPEED, retv);
  // FIXME
  // if (retv != 0) exit(EXIT_FAILURE);

  // creade listen IRQ threads
  vsthread_create(32, SCHED_FIFO, &thread_irq, thread_irq_fn, NULL);
  
  // set handler for SIGINT (CTRL+C)
  retv = stimer_sigint(sigint_handler, (void*) NULL);
  printf(">>> stimer_sigint_handler() return %d\n", retv);
  if (retv != 0) exit(EXIT_FAILURE);

  // setup SX127x module
  sx127x_init(
      &radio,
      SX127X_LORA,     // radio mode: 0 - LoRa, 1 - FSK, 2 - OOK
      on_spi_cs,       // SPI crystal select function                          
      on_spi_exchange, // SPI exchange function (return number or RX bytes)
      on_receive,      // receive callback or NULL
      (const sx127x_pars_t *) NULL, // configuration parameters or NULL
      (void*) &spi,    // optional SPI exchange context
      (void*) NULL);   // optional on_receive() context

  // set "real-time" priority
  if (1)
  {
    retv = stimer_realtime();
    printf(">>> stimer_realtime() return %d\n", retv);
  }

  // setup timer
  retv = stimer_init(&timer, timer_handler, (void*) NULL);
  printf(">>> stimer_init() return %d\n", retv);
  if (retv != 0) exit(EXIT_FAILURE);

  // run periodic timer
  retv = stimer_start(&timer, interval);
  printf(">>> stimer_start(%f) return %d\n", interval, retv);
  if (retv != 0) exit(EXIT_FAILURE);

  // hard reset SX127x radio module
  reset_radio();

  // run timer loop ()
  retv = stimer_loop(&timer);
  printf(">>> stimer_loop() return %i\n", retv);

  // free SX127x module
  sx127x_free(&radio);

  // free SPI
  spi_free(&spi);

  // unexport GPIOs
  sgpio_free(&gpio_led);
  sgpio_free(&gpio_data);
  sgpio_free(&gpio_cs);
  sgpio_free(&gpio_reset);
  sgpio_free(&gpio_irq);
  if (1)
  {
    sgpio_unexport(GPIO_LED);
    sgpio_unexport(GPIO_DATA);
    sgpio_unexport(GPIO_CS);
    sgpio_unexport(GPIO_RESET);
    sgpio_unexport(GPIO_IRQ);
  }

  return EXIT_SUCCESS;
}
//-----------------------------------------------------------------------------

/*** end of "sx127x_test.c" file ***/

