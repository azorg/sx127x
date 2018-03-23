Semtech SX127x famaly chips driver
==================================

## Supperted radio modes

* LoRaTM (RX/TX)

* FSK/GFSK (RX/TX, packet/continuous)

* OOK (RX/TX, packet/continuous)

* CW (OOK continuous mode, external modulation, TX only)

* Slow FM (set frequency by SPI in continuous mode, TX only)

## Fetchers

- set RF frequency

- set output power (PA_BOOST pi, RFO pin, HighPower - up to +20 dBm on PA_BOOST pin)

- set almost all LoRaTM/FSK/OOK signal parameters and options

- set OCP trimmer

- simple fast start with default parameters

- portable open source code

## Main functions

* sx127x_init() - constructor

* sx127x_pars() - set all parameters from `sx127x_pars_t` structure

* sx127x_send() - send message in packet mode (LoRa/FSK/OOK)

* sx127x_receive() - go to receive (RX) mode

* sx127x_set_fast_hop() - set fast hopping for program Slow FM by SPI

* sx127x_set_frequency() - set RF frequency [Hz]

* sx127x_set_power_***() - set output power (some function) [Hz]

* sx127x_set_high_power() - set High Power (+3 dB on PA_BOOST pin)

* sx127x_set_ocp() - setup OCP

* sx127s_set_crc() - on/off CRC in packet LoRa/FSK/OOK modes

* sx127x_continuous() - continuous mode (no packet)

Look "sx127x.h" header file for details.

## Files

- "sx127x_defs" - SX127x chip define's (may not includes in users program)

- "sx127x.h" - main header to include in users programm

- "sx127x.c" - main compilation unit of this module

- "README.md" - this file



