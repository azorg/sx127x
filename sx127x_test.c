/*
 * -*- coding: UTF8 -*-
 * Semtech SX127x famaly chips driver (test/example module)
 * File: "sx127x_test.h"
 */

//-----------------------------------------------------------------------------
#include <stdlib.h>     // exit(), EXIT_SUCCESS, EXIT_FAILURE
#include <string.h>     // strlen()
#include "spi.h"        // `spi_t`
#include "sgpio.h"      // `sgpio_t`
#include "stimer.h"     // `stimer_t`
#include "vsthread.h"   // `vsthread.h`
#include "sx127x.h"     // `sx127x_t`
#include "sx127x_def.h" // SX127x define's
//-----------------------------------------------------------------------------
// demo mode
#define DEMO_MODE 3 // 1 - transmitter, 2 - receiver, 3 - morse beeper

// radio mode
#define RADIO_MODE 1 // 0 - LoRa, 1 - FSK, 2 - OOK

// timer interval
#define TIMER_INTERVAL 1000 // ms

//-----------------------------------------------------------------------------
#define ORANGE_PI_ZERO
//#define ORANGE_PI_ONE
//#define ORANGE_PI_WIN_PLUS

// GPIO lines and SPI devic to SX127x module
#if defined(ORANGE_PI_ZERO)
#  define SPI_DEVICE  "/dev/spidev1.0" // on linux ver. 3.4.113-sun8i
#  define GPIO_IRQ     6  // pin  7 of 26 (GPIO.7)
//#  define GPIO_IRQ   1  // pin 11 of 26 (RxD2)
#  define GPIO_RESET   7  // pin 12 of 26 (GPIO.1)
#  define GPIO_CS      13 // pin 24 of 26 (CE0)
#  define GPIO_DATA    19 // pin 16 of 26 (GPIO.4)
#  define GPIO_LED     18 // pin 18 of 26 (GPIO.5)
//#  define GPIO_LED   3  // pin 15 of 26 (CTS2)
//#  define GPIO_LED   2  // pin 22 of 26 (RTS2)
#elif defined(ORANGE_PI_ONE)
#  define SPI_DEVICE "/dev/spidev1.0" // FIXME
#  define GPIO_IRQ   1 // FIXME
#  define GPIO_RESET 2 // FIXME
#  define GPIO_CS    3 // FIXME
#  define GPIO_DATA  4 // FIXME
#  define GPIO_LED   5 // FIXME
#elif defined(ORANGE_PI_WIN_PLUS)
#  define SPI_DEVICE "/dev/spidev1.0" // FIXME
#  define GPIO_IRQ   1 // FIXME
#  define GPIO_RESET 2 // FIXME
#  define GPIO_CS    3 // FIXME
#  define GPIO_DATA  4 // FIXME
#  define GPIO_LED   5 // FIXME
#endif

// SPI max speed [Hz]
#define SPI_SPEED 20000000 // 20 MHz 
//-----------------------------------------------------------------------------
// global variables
spi_t spi;
int global_stop = 0; // global stop flag
double interval = TIMER_INTERVAL; // [ms]
stimer_t timer;
vsthread_t thread_irq;
sx127x_t radio;
int demo_mode = DEMO_MODE;

#ifdef GPIO_IRQ
sgpio_t  gpio_irq;   // in IRQ
#endif // GPIO_IRQ

#ifdef GPIO_RESET
sgpio_t  gpio_reset; // out
#endif // GPIO_RESET

#ifdef GPIO_CS
sgpio_t  gpio_cs;    // out
#endif // GPIO_CS

#ifdef GPIO_DATA
sgpio_t  gpio_data;  // out
#endif // GPIO_DATA

#ifdef GPIO_LED
sgpio_t  gpio_led;   // out
#endif // GPIO_LED

//-----------------------------------------------------------------------------
// SIGINT handler (Ctrl-C)
static void sigint_handler(void *context)
{
  global_stop = 1;
  stimer_stop(&timer);
  fprintf(stderr, "\nCtrl-C pressed\n");
} 
//-----------------------------------------------------------------------------
// on/off DATA
void data_on(int on)
{
#ifdef GPIO_DATA
  sgpio_set(&gpio_data, on ? 1 : 0);
#endif // GPIO_DATA
}
//-----------------------------------------------------------------------------
// on/off LED
void led_on(int on)
{
#ifdef GPIO_LED
  sgpio_set(&gpio_led, on ? 1 : 0);
#endif // GPIO_LED
}
//-----------------------------------------------------------------------------
// blink LED
void blink_led()
{
#ifdef GPIO_LED
  led_on(1);
  stimer_sleep_ms(100.);
  led_on(0);
  stimer_sleep_ms(20.);
#else
  printf(">>> blink_led() :-)\n");
#endif // GPIO_LED
}
//-----------------------------------------------------------------------------
// IRQ waiting thread
static void *thread_irq_fn(void *arg)
{
  printf(">>> start irq_thread()\n");
  
#ifdef GPIO_IRQ
  while (1)
  {
    // wait interrupt
    int retv  = sgpio_poll(&gpio_irq, 1000); // FIXME
    int value = sgpio_get( &gpio_irq);
    //printf(">>> sgpio_poll(&gpio_irq, 1000) return %d\n", retv); // FIXME

    if (retv > 0)
    { // interrupt
      //printf(">>> sgpio_get(&gpio_irq) return %d\n", value); // FIXME
      if (value) sx127x_irq_handler(&radio);
    }
    else if (retv == 0)
    { // timeout
      if (global_stop)
      {
        printf(">>> thread_irq_fn() finished by `global_stop`"); // FIXME
        break; // finish by `global_stop`
      }
    }
    else // retv < 0
    { // error
      printf(">>> thread_irq_fn() finished by sgpio_poll() error"); // FIXME
      global_stop = 1;
      break; // finish by sgpio_poll() error
    }
  } // while(1)
#endif // GPIO_IRQ

  return NULL;
}
//-----------------------------------------------------------------------------
// SPI crystal select by GPIO wrapper function
static inline void spi_cs(bool cs)
{
#ifdef GPIO_CS
  sgpio_set(&gpio_cs, cs ? 0 : 1); // negative CS
#endif // GPIO_CS
}
//-----------------------------------------------------------------------------
// set of data output port (GPIO_DATA)
static inline void radio_data(bool value)
{
#ifdef GPIO_DATA
  printf(">>> dadio_data(%s)\n", value ? "1" : "0"); // FIXME
  sgpio_set(&gpio_cs, (int) value);
#endif // GPIO_DATA
}
//-----------------------------------------------------------------------------
// hard reset SX127x radio module
void reset_radio()
{
  printf(">>> reset_radio()\n");

#ifdef GPIO_RESET
  sgpio_set(&gpio_reset, 1);
  stimer_sleep_ms(100.);
  sgpio_set(&gpio_reset, 0);
  stimer_sleep_ms(100.);
  sgpio_set(&gpio_reset, 1);
  stimer_sleep_ms(100.);
#endif // GPIO_RESET
}
//-----------------------------------------------------------------------------
// SPI exchange wrapper function (return number or RX bytes)
int on_spi_exchange(
  u8_t       *rx_buf, // RX buffer
  const u8_t *tx_buf, // TX buffer
  u8_t len,           // number of bytes
  void *context)      // optional SPI context or NULL
{
  int retv;
  spi_t *spi = (spi_t*) context; 

  spi_cs(true);  // select crystall
  retv = spi_exchange(spi, (char*) rx_buf, (const char*) tx_buf, (int) len);
  spi_cs(false); // unselect crystall

#if 0
  // FIXME SPI debug
  printf(">>> spi_exchange([0x%02X, 0x%02X], len=%d) "
         "return ([0x%02X, 0x%02X], retv=%d)\n",
         tx_buf[0], tx_buf[1], (int) len,
         rx_buf[0], rx_buf[1], (int) retv);
#endif

  return retv;
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
  int i;
  int16_t rssi = sx127x_get_rssi(&radio);
  int16_t snr  = sx127x_get_snr(&radio);
  
  blink_led();
  printf("*** Received message: ***\n");

  for (i = 0; i < payload_size; i++)
    printf("%c", (char) payload[i]);

  printf("\n^^^ crcOk=%s, RSSI=%d, SNR=%d ^^^\n",
        crc ? "true" : "false", rssi, snr);

}
//-----------------------------------------------------------------------------
// periodic timer handler (main periodic function)
static int timer_handler(void *context)
{
  u8_t version = sx127x_version(&radio);
  i16_t rssi   = sx127x_get_rssi(&radio);

  //printf(">>> start timer_handrer()\n");
  printf(">>> SX127x selicon revision = 0x%02X\n", (unsigned) version);
  printf(">>> RSSI = %d dBm\n", rssi); 

  if (demo_mode == 1)
  { // transmitter
    char *str = "Hello!";
    printf(">>> sx127x_send('%s')\n", str);
    led_on(1);
    sx127x_send(&radio, (u8_t*) str, strlen(str));
    led_on(0);
  }
  else if (demo_mode == 2)
  { // receiver
    // do nothing
  }
  else if (demo_mode == 3)
  { // morse beeper
    sx127x_tx(&radio);
    
    led_on(1);
    data_on(1);

    stimer_sleep_ms(100); // FIXME
    
    led_on(0);
    data_on(0);

    sx127x_standby(&radio);
  }

  return 0;
}
//-----------------------------------------------------------------------------
// simple example usage of `sx127x_t` component 
int main()
{
  int retv, radio_mode = RADIO_MODE;
 
  // set "real-time" priority
  if (1)
  {
    retv = stimer_realtime();
    printf(">>> stimer_realtime() return %d\n", retv);
  }

  // setup GPIOs
  if (1)
  {
#ifdef GPIO_IRQ
    sgpio_unexport(GPIO_IRQ); // FIXME
    sgpio_export(GPIO_IRQ);
#endif // GPIO_IRQ

#ifdef GPIO_RESET
    sgpio_export(GPIO_RESET);
#endif // GPIO_RESET

#ifdef GPIO_CS
    sgpio_export(GPIO_CS);
#endif // GPIO_CS

#ifdef GPIO_DATA
    sgpio_export(GPIO_DATA);
#endif // GPIO_DATA

#ifdef GPIO_LED
    sgpio_export(GPIO_LED);
#endif // GPIO_LED
  }

#ifdef GPIO_IRQ
  sgpio_init(&gpio_irq,   GPIO_IRQ);
  sgpio_mode(&gpio_irq,   SGPIO_DIR_IN,  SGPIO_EDGE_RISING); // FIXME

  retv = sgpio_get(&gpio_irq);
  printf(">>> first sgpio_get(&gpio_irq) return %d\n", retv); // FIXME
#endif // GPIO_IRQ
  
#ifdef GPIO_RESET
  sgpio_init(&gpio_reset, GPIO_RESET);
  sgpio_mode(&gpio_reset, SGPIO_DIR_OUT, SGPIO_EDGE_NONE);
#endif // GPIO_RESET

#ifdef GPIO_CS
  sgpio_init(&gpio_cs,    GPIO_CS);
  sgpio_mode(&gpio_cs,    SGPIO_DIR_OUT, SGPIO_EDGE_NONE);
  spi_cs(false);
#endif // GPIO_CS

#ifdef GPIO_DATA
  sgpio_init(&gpio_data,  GPIO_DATA);
  sgpio_mode(&gpio_data,  SGPIO_DIR_OUT, SGPIO_EDGE_NONE);
  radio_data(0);
#endif // GPIO_DATA

#ifdef GPIO_LED
  sgpio_init(&gpio_led,   GPIO_LED);
  sgpio_mode(&gpio_led,   SGPIO_DIR_OUT, SGPIO_EDGE_NONE);
#endif // GPIO_LED

  // setup SPI
  retv = spi_init(&spi,
                  SPI_DEVICE, // filename like "/dev/spidev0.0"
                  0,          // SPI_* (look "linux/spi/spidev.h")
                  0,          // bits per word (usually 8)
                  SPI_SPEED); // max speed [Hz]
  printf(">>> spi_init(device='%s', speed=%d) return %d\n",
         SPI_DEVICE, SPI_SPEED, retv);
  if (retv != 0) exit(EXIT_FAILURE);

  // creade listen IRQ threads
  vsthread_create(32, SCHED_FIFO, &thread_irq, thread_irq_fn, NULL);
  
  // set handler for SIGINT (CTRL+C)
  retv = stimer_sigint(sigint_handler, (void*) NULL);
  printf(">>> stimer_sigint_handler() return %d\n", retv);
  if (retv != 0) exit(EXIT_FAILURE);

  // hard reset SX127x radio module
  reset_radio();

  if (demo_mode == 3)
    radio_mode = SX127X_OOK; // OOK in morse beeper demo

  // setup SX127x module
  sx127x_init(
      &radio,
      radio_mode,      // radio mode: 0 - LoRa, 1 - FSK, 2 - OOK
      on_spi_exchange, // SPI exchange function (return number or RX bytes)
      on_receive,      // receive callback or NULL
      (const sx127x_pars_t *) NULL, // configuration parameters or NULL
      (void*) &spi,    // optional SPI exchange context
      (void*) NULL);   // optional on_receive() context

  // common settings
  sx127x_set_frequency(&radio, 434000000); // RF frequency [Hz]
  sx127x_set_power_dbm(&radio, 17);        // power +17dBm
  sx127x_set_high_power(&radio, false);    // add +3 dB on PA_BOOST pin
  sx127x_set_ocp(&radio, 230, true);       // set OCP trimming [mA]
  sx127x_enable_crc(&radio, true, true);   // CRC=on, CrcAutoClearOff=on
  sx127x_set_pll_bw(&radio, 1);            // 0=75, 1=150, 2=225, 3=300 kHz

  if (sx127x_is_lora(&radio))
  { // LoRa settings
    sx127x_set_bw(      &radio, 250000); // BW: 78000...500000 Hz
    sx127x_set_cr(      &radio, 5);      // CR: 5..8
    sx127x_set_sf(      &radio, 7);      // SF: 6...12
    sx127x_set_ldro(    &radio, false);  // Low Datarate Optimize
    sx127x_set_preamble(&radio, 6);      // 6..65535 (8 by default)
    sx127x_set_sw(      &radio, 0x12);   // SW allways 0x12
  }
  else
  { // FSK/OOK settings
    sx127x_set_bitrate(&radio, 4800, -1); // bit/s, no frac
    sx127x_set_fdev(   &radio, 5000);     // frequency deviation [Hz]
    sx127x_set_rx_bw(  &radio, 10400);    // RX BW [Hz]
    sx127x_set_afc_bw( &radio, 2600);     // AFC BW [Hz]
    sx127x_set_afc(    &radio, true);     // AFC on/off
    sx127x_set_fixed(  &radio, false);    // fixed packet size or variable
    sx127x_set_dcfree( &radio, 0);        // 0=Off, 1=Manchester, 2=Whitening
  }

  // preapre to run one of demo application
  if (demo_mode == 1)
  { // transmitter
    // UNSET!!! callback on receive packet (Lora/FSK/OOK)
    sx127x_on_receive(&radio, NULL, NULL); // FIXME
  }
  else if (demo_mode == 2)
  { // receiver
    // set !!!AGAIN!!! callback on receive packet (Lora/FSK/OOK)
    sx127x_on_receive(&radio, on_receive, NULL);

    // go to receive mode
    sx127x_receive(&radio, 255);
  }
  else if (demo_mode == 3)
  { // morse beeper
    data_on(0); // off OOK modulation
    sx127x_set_fast_hop(&radio, true);
    sx127x_continuous(&radio, true); // switch to continuous mode
  }

  // setup timer
  retv = stimer_init(&timer, timer_handler, (void*) NULL);
  printf(">>> stimer_init() return %d\n", retv);
  if (retv != 0) exit(EXIT_FAILURE);

  // run periodic timer
  retv = stimer_start(&timer, interval);
  printf(">>> stimer_start(%f) return %d\n", interval, retv);
  if (retv != 0) exit(EXIT_FAILURE);

  // run timer loop ()
  retv = stimer_loop(&timer);
  printf(">>> stimer_loop() return %i\n", retv);

  // free SX127x module
  sx127x_free(&radio);

  // free SPI
  spi_free(&spi);

  // unexport GPIOs
#ifdef GPIO_LED
  sgpio_free(&gpio_led);
#endif // GPIO_LED

#ifdef GPIO_DATA
  sgpio_free(&gpio_data);
#endif // GPIO_DATA

#ifdef GPIO_CS
  sgpio_free(&gpio_cs);
#endif // GPIO_CS

#ifdef GPIO_RESET
  sgpio_free(&gpio_reset);
#endif // GPIO_RESET

#ifdef GPIO_IRQ
  sgpio_free(&gpio_irq);
#endif // GPIO_IRQ

  if (1)
  {
#ifdef GPIO_LED
    sgpio_unexport(GPIO_LED);
#endif // GPIO_LED

#ifdef GPIO_DATA
    sgpio_unexport(GPIO_DATA);
#endif // GPIO_DATA

#ifdef GPIO_CS
    sgpio_unexport(GPIO_CS);
#endif // GPIO_CS

#ifdef GPIO_RESET
    sgpio_unexport(GPIO_RESET);
#endif // GPIO_RESET

#ifdef GPIO_IRQ
    sgpio_unexport(GPIO_IRQ);
#endif // GPIO_IRQ
  }

  return EXIT_SUCCESS;
}
//-----------------------------------------------------------------------------

/*** end of "sx127x_test.c" file ***/

