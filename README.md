Connect Ra-01/Ra-02 module base on LoRaTM SX127x chip to Orange Pi
==================================================================

### Notes
1. This is experimental example, not software product ready for use
2. This this free and open source software
3. Author: Alex Zorg azorg(at)mail.ru
4. Licenced by GPLv3
5. Tested on Orange Pi Zero, Orange Pi Win Plus (in plans)
6. Sources based on:
  * https://github.com/azorg/sx127x_esp
  * https://github.com/azorg/spi
  * https://github.com/azorg/sgpio
  * https://github.com/azorg/vsrpc

## Connect LoRa SX1278 module to you Orange Pi

|   GPIO   | Pi Zero    | Pi Win Plus |   Signal    |  SX1278 (color)  |
| -------- | ---------- | ----------- | ----------- | ---------------- |
|          |            |             |    IRQ      | DIO0   (yellow)  |
|          |            |             | HARD RESET  | RESET  (magenta) |
|          |            |             |    MISO     | MISO   (blue)    |
|          |            |             |    MOSI     | MOSI   (green)   |
|          |            |             |    SCK      | SCK    (white)   |
|          |            |             |    CS       | NSS    (grey)    |
|          |            |             |    DATA*    | DIO2*  (brown)   |
|    -     |    -       |     -       |    DCLK**   | DIO1** (orange)  |
|          |            |             |    3.3V     | 3.3V   (red)     |
|          |            |             |    GND      | GND    (black)   |

Note (*):  DIO2(DATA) is optional and may used in continuous FSK/OOK mode

Note (**): DIO1(DCLK) unused

## Build test application

```
$ make
```

