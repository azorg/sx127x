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

## Orange Pi Zero 26 pin connector

 | GPIO | Signal |Pin |Pin | Signal  | GPIO |
 |:----:| ------:|:--:|:--:|:------- |:----:|
 |      |   3.3V |  1 | 2  | 5V      |      |
 |  12  |  SDA.0 |  3 | 4  | 5V      |      |
 |  11  |  SCL.0 |  5 | 6  | 0V      |      |
 |   6  | GPIO.7 |  7 | 8  | TxD1    | 198  |
 |      |     0V |  9 | 10 | RxD1    | 199  | 
 |   1  |   RxD2 | 11 | 12 | GPIO.1  | 7    |
 |   0  |   TxD2 | 13 | 14 | 0V      |      |
 |   3  |   CTS2 | 15 | 16 | GPIO.4  | 19   |
 |      |   3.3v | 17 | 18 | GPIO.5  | 18   |
 |  15  |   MOSI | 19 | 20 | 0V      |      |
 |  16  |   MISO | 21 | 22 | RTS2    | 2    |
 |  14  |   SCLK | 23 | 24 | CE0     | 13   |
 |      |     0v | 25 | 26 | GPIO.11 | 10   |

## Orange Pi Win Plus 40 pin connector
...

## Connect LoRa SX1278 module to you Orange Pi

|   GPIO   |    Pi Zero   | Pi Win Plus |   Signal    | SX1278 (color)  |
|:--------:|:------------:|:-----------:|:----------- |:--------------- |
|     6    |       7      |             | IRQ         | DIO0  (yellow)  |
|     7    |      12      |             | HARD RESET  | RESET (magenta) |
|    15    |      19      |             | MOSI        | MOSI  (green)   |
|    16    |      21      |             | MISO        | MISO  (blue)    |
|    14    |      23      |             | SCK         | SCK   (white)   |
|    18    |      18      |             | CS          | NSS   (grey)    |
|    19    |      16      |             | DATA *      | DIO2  (brown)   |
|    10    |      26      |             | DCLK **     | DIO1  (orange)  |
|    13    |      24      |             | LED         | -               |
|    -     |   1 or 17    |             | 3.3V        | 3.3V  (red)     |
|    -     | 25,20,14,9,6 |             | GND         | GND   (black)   |

Note (*):  DIO2(DATA) is optional and may used in continuous FSK/OOK mode

Note (**): DIO1(DCLK) unused

## Build test application

```
$ make
```

