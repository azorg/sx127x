/*
 * -*- coding: UTF8 -*-
 * Semtech SX127x famaly chips driver (test/example module)
 * File: "sx127x_test.h"
 */

//-----------------------------------------------------------------------------
#include <stdio.h>      // printf(), NULL
#include <string.h>     // strlen()
#include "stimer.h"     // `stimer_t`
#include "radio.h"      // `sx127x_t`, radio_*()
#include "sx127x_def.h" // SX127x define's
#include <stdlib.h>     // exit(), EXIT_SUCCESS, EXIT_FAILURE
//-----------------------------------------------------------------------------
// demo mode
#define DEMO_MODE 1 // 0 - transmitter, 1 - receiver, 2 - morse beeper

// radio mode
#define RADIO_MODE 1 // 0 - LoRa, 1 - FSK, 2 - OOK

// timer interval
#define TIMER_INTERVAL 1000 // ms

// implicit header (LoRa) or fixed packet length (FSK/OOK)
//#define FIXED

//-----------------------------------------------------------------------------
stimer_t timer;
int demo_mode = DEMO_MODE;
//-----------------------------------------------------------------------------
// SIGINT handler (Ctrl-C)
static void sigint_handler(void *context)
{
  radio_stop = 1;
  stimer_stop(&timer);
  printf("\nCtrl-C pressed\n");
} 
//-----------------------------------------------------------------------------
// receive callback
static void on_receive(
    sx127x_t *self,     // pointer to sx127x_t object
    u8_t *payload,      // payload data
    u8_t payload_size,  // payload size
    bool crc,           // CRC ok/false
    void *context)      // optional context
{
  int i;
  int16_t rssi = sx127x_get_rssi(&radio);
  int16_t snr  = sx127x_get_snr(&radio);
  
  radio_blink_led();
  printf("*** Received message:\n");

  for (i = 0; i < payload_size; i++)
#if 1
    printf("%c", (char) payload[i]);
#else
    printf("%02X ", payload[i]);
#endif

  printf("\n^^^ CrcOk=%s, size=%i, RSSI=%d, SNR=%d\n",
        crc ? "true" : "false", payload_size, rssi, snr);

}
//-----------------------------------------------------------------------------
// periodic timer handler (main periodic function)
static int timer_handler(void *context)
{
  if (demo_mode == 0)
  { // transmitter
    char *str = "Hello!";
    printf(">>> sx127x_send('%s')\n", str);
    //radio_led_on(true);
#ifdef FIXED
    sx127x_send(&radio,
                (u8_t*) str, strlen(str), true); // implicit header / fixed
#else
    sx127x_send(&radio,
                (u8_t*) str, strlen(str), false); // explicit header / varible
#endif
    //radio_led_on(false);
  }
  else if (demo_mode == 1)
  { // receiver
    i16_t rssi = sx127x_get_rssi(&radio);
    printf(">>> RSSI = %d dBm\n", rssi); 
  }
  else if (demo_mode == 2)
  { // morse beeper
    sx127x_tx(&radio);
    
    printf(">>> *** BEEP ***\n"); 
    radio_led_on(1);
    radio_data_on(1);
    stimer_sleep_ms(500); // FIXME
    radio_led_on(0);
    radio_data_on(0);

    sx127x_standby(&radio);
  }

  return 0;
}
//-----------------------------------------------------------------------------
// simple example usage of `sx127x_t` component 
int main()
{
  int retv, radio_mode = RADIO_MODE;
  u8_t reg;
  u32_t freq;
 
  // set "real-time" priority
  if (1)
  {
    retv = stimer_realtime();
    printf(">>> stimer_realtime() return %d\n", retv);
  }

  // set handler for SIGINT (CTRL+C)
  retv = stimer_sigint(sigint_handler, (void*) NULL);
  printf(">>> stimer_sigint_handler() return %d\n", retv);
  if (retv != 0) exit(EXIT_FAILURE);

  if (demo_mode == 2)
    radio_mode = SX127X_OOK; // OOK in morse beeper demo

  // init SX127x radio module hardware layer
  radio_init();

  // setup SX127x module
  retv = sx127x_init(
      &radio,
      radio_mode,         // radio mode: 0 - LoRa, 1 - FSK, 2 - OOK
      radio_spi_exchange, // SPI exchange function (return number or RX bytes)
      on_receive,         // receive callback or NULL
      (const sx127x_pars_t *) NULL, // configuration parameters or NULL
      (void*) NULL,       // optional SPI exchange context
      (void*) NULL);      // optional on_receive() context
  printf(">>> sx127x_init() return %d\n", retv);

  // common settings
  sx127x_set_frequency(&radio, 434000000); // RF frequency [Hz]
  sx127x_set_power_dbm(&radio, 10);        // power [dBm]
  sx127x_set_high_power(&radio, false);    // add +3 dB on PA_BOOST pin
  sx127x_set_ocp(&radio, 120, true);       // set OCP trimming [mA]
  sx127x_enable_crc(&radio, true, true);   // CRC, CrcAutoClearOff
  sx127x_set_pll_bw(&radio, 2);            // 0=75, 1=150, 2=225, 3=300 kHz

  if (sx127x_is_lora(&radio))
  { // LoRa settings
    sx127x_set_bw(      &radio, 250000); // BW: 78000...500000 Hz
    sx127x_set_cr(      &radio, 5);      // CR: 5..8
    sx127x_set_sf(      &radio, 7);      // SF: 6...12
    sx127x_set_ldro(    &radio, false);  // Low Datarate Optimize
    sx127x_set_preamble(&radio, 6);      // 6..65535 (8 by default)
    sx127x_set_sw(      &radio, 0x12);   // SW allways 0x12
    sx127x_impl_hdr(    &radio, false);  // explicit header
  }
  else
  { // FSK/OOK settings
    sx127x_set_bitrate(&radio, 4800);  // bit/s
    sx127x_set_fdev(   &radio, 5000);  // frequency deviation [Hz]
    sx127x_set_rx_bw(  &radio, 10400); // RX BW [Hz]
    sx127x_set_afc_bw( &radio, 2600);  // AFC BW [Hz]
    sx127x_set_afc(    &radio, true);  // AFC on/off
    sx127x_set_fixed(  &radio, false); // fixed packet size or variable
    sx127x_set_dcfree( &radio, 0);     // 0=Off, 1=Manchester, 2=Whitening
  }

  // get RF frequency [Hz]
  freq = sx127x_get_frequency(&radio);
  printf(">>> sx127x_get_frequency() return %lu Hz\n", freq);

  // get current RX gain code [1..6] from `RegLna` (1 - maximum gain)
  reg = sx127x_get_rx_gain(&radio);
  printf(">>> sx127x_get_rx_gain() return %d\n", (int) reg);

  // dump registers
  //sx127x_dump(&radio);

  // preapre to run one of demo application
  if (demo_mode == 0)
  { // transmitter
    // UNSET callback on receive packet (Lora/FSK/OOK)
    //sx127x_on_receive(&radio, NULL, NULL); // FIXME
  }
  else if (demo_mode == 1)
  { // receiver
    // set !!!AGAIN!!! callback on receive packet (Lora/FSK/OOK)
    sx127x_on_receive(&radio, on_receive, NULL);

    // go to receive mode
#ifdef FIXED
    sx127x_receive(&radio, 6); // 6=size("Hello!")
#else
    sx127x_receive(&radio, 0); // explicit header or variable packet length
#endif
  }
  else if (demo_mode == 2)
  { // morse beeper
    //sx127x_set_fast_hop(&radio, true); // FIXME
    sx127x_continuous(&radio, true); // switch to continuous mode
  }

  // setup timer
  retv = stimer_init(&timer, timer_handler, (void*) NULL);
  printf(">>> stimer_init() return %d\n", retv);
  if (retv != 0) exit(EXIT_FAILURE);

  // run periodic timer
  retv = stimer_start(&timer, (double) TIMER_INTERVAL);
  printf(">>> stimer_start(%f) return %d\n", (double) TIMER_INTERVAL, retv);
  if (retv != 0) exit(EXIT_FAILURE);

  // run timer loop ()
  retv = stimer_loop(&timer);
  printf(">>> stimer_loop() return %i\n", retv);

  // free SX127x radio module
  radio_free();

  return EXIT_SUCCESS;
}
//-----------------------------------------------------------------------------

/*** end of "sx127x_test.c" file ***/

