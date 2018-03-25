/*
 * -*- coding: UTF8 -*-
 * Semtech SX127x famaly chips driver
 * File: "sx127x_def.h"
 */

#ifndef SX127X_DEF_H
#define SX127X_DEF_H
//-----------------------------------------------------------------------------
// Common registers
#define REG_FIFO      0x00 // FIFO read/write access
#define REG_OP_MODE   0x01 // Operation mode & LoRaTM/FSK/OOK selection
#define REG_FRF_MSB   0x06 // RF Carrier Frequency, MSB
#define REG_FRF_MID   0x07 // RF Carrier Frequency, Mid
#define REG_FRF_LSB   0x08 // RF Carrier Frequency, LSB
#define REG_PA_CONFIG 0x09 // PA selection and Output power control
#define REG_PA_RAMP   0x0A // Controll of PA ramp time, low phase noise PLL
#define REG_OCP       0x0B // Over Current Protection (OCP) control 
#define REG_LNA       0x0C // LNA settings

#define REG_DIO_MAPPING_1 0x40 // Mapping of pins DIO0 to DIO3
#define REG_DIO_MAPPING_2 0x41 // Mapping of pins DIO4, DIO5, CLK-OUT frequency
#define REG_VERSION       0x42 // Semtech ID relating the silicon revision

#define REG_TCXO        0x4B // TCXO or XTAL input settings
#define REG_PA_DAC      0x4D // Higher power settings of the PA
#define REG_FORMER_TEMP 0x5B // Stored temperature during the former IQ Calibration

#define REG_AGC_REF      0x61 // Adjustment of the AGC threshold
#define REG_AGC_THRESH_1 0x62 // ...
#define REG_AGC_THRESH_2 0x63 // ...
#define REG_AGC_THRESH_3 0x64 // ...

#define REG_PLL 0x70 // Contral of the PLL bandwidth

// FSK/OOK mode registers
#define REG_BITRATE_MSB     0x02 // Bit rate settings, MSB
#define REG_BITRATE_LSB     0x03 // Bit rate settings, LSB
#define REG_FDEV_MSB        0x04 // Frequency Deviation settings, MSB (FSK)
#define REG_FDEV_LSB        0x05 // Frequency Deviation settings, LSB (FSK)

#define REG_RX_CONFIG       0x0D // AFC, AGC, ctrl
#define REG_RSSI_CONFIG     0x0E // RSSI
#define REG_RSSI_COLLISION  0x0F // RSSI Collision detector
#define REG_RSSI_TRESH      0x10 // RSSI Treshhold control
#define REG_RSSI_VALUE      0x11 // RSSI value in dBm (0.5 dB steps)
#define REG_RX_BW           0x12 // Channel Filter BW control
#define REG_AFC_BW          0x13 // AFC channel filter BW
#define REG_OOK_PEAK        0x14 // OOK demodulator
#define REG_OOK_FIX         0x15 // Treshold of the OOK demod
#define REG_OOK_AVG         0x16 // Average of the OOK demod

#define REG_AFC_FEI         0x1A // AFC and FEI control
#define REG_AFC_MSB         0x1B // Frequency correction value of the AFC, MSB
#define REG_AFC_LSB         0x1C // Frequency correction value of the AFC, LSB
#define REG_FEI_MSB         0x1D // Value of the calculated frequency error, MSB
#define REG_FEI_LSB         0x1E // Value of the calculated frequency error, LSB
#define REG_PREAMBLE_DETECT 0x1F // Settings of preamble Detector
#define REG_RX_TIMEOUT_1    0x20 // Timeout Rx request and RSSI
#define REG_RX_TIMEOUT_2    0x21 // Timeout RSSI and PayloadReady
#define REG_RX_TIMEOUT_3    0x22 // Timeout RSSI and SyncAddress
#define REG_RX_DELAY        0x23 // Delay between Rx cycles
#define REG_OSC             0x24 // RC Oscillators Settings, CLK-OUT frequency
#define REG_PREAMBLE_L_MSB  0x25 // Preampbe length, MSB 
#define REG_PREAMBLE_L_LSB  0x26 // Preampbe length, LSB
#define REG_SYNC_CONFIG     0x27 // Sync Word Recognition control
#define REG_SYNC_VALUE_1    0x28 // Sync Word byte 1
#define REG_SYNC_VALUE_2    0x29 // Sync Word byte 2
#define REG_SYNC_VALUE_3    0x2A // Sync Word byte 3
#define REG_SYNC_VALUE_4    0x2B // Sync Word byte 4
#define REG_SYNC_VALUE_5    0x2C // Sync Word byte 5
#define REG_SYNC_VALUE_6    0x2D // Sync Word byte 6
#define REG_SYNC_VALUE_7    0x2E // Sync Word byte 7
#define REG_SYNC_VALUE_8    0x2F // Sync Word byte 8
#define REG_PACKET_CONFIG_1 0x30 // Packet mode settings
#define REG_PACKET_CONFIG_2 0x31 // Packet mode settings
#define REG_PAYLOAD_LEN     0x32 // Payload lenght settings
#define REG_NODE_ADRS       0x33 // Node address
#define REG_BROADCAST_ADRS  0x34 // Broadcast address
#define REG_FIFO_THRESH     0x35 // FIFO Theshold, Tx start condition
#define REG_SEQ_CONFIG_1    0x36 // Top level Sequencer settings
#define REG_SEQ_CONFIG_2    0x37 // Top level Sequencer settings
#define REG_TIMER_RESOL     0x38 // Timer 1 and 2 resolution control
#define REG_TIMER_1_COEF    0x39 // Timer 1 settings
#define REG_TIMER_2_COEF    0x3A // Timer 2 settings
#define REG_IMAGE_CAL       0x3B // Image callibration engine control
#define REG_TEMP            0x3C // Tempreture Sensor value
#define REG_LOW_BAT         0x3D // Low Battary Indicator Settings
#define REG_IRQ_FLAGS_1     0x3E // Status register: PLL lock state, Timeout, RSSI
#define REG_IRQ_FLAGS_2     0x3F // Status register: FIFO handing, flags, Low Battery

#define REG_PLL_HOP      0x44 // Control the fast frequency hopping mode
#define REG_BITRATE_FRAC 0x5D // Fraction part in the Bit Rate division ratio

// LoRaTM mode registers
#define REG_FIFO_ADDR_PTR        0x0D // FIFO SPI pointer
#define REG_FIFO_TX_BASE_ADDR    0x0E // Start Tx data
#define REG_FIFO_RX_BASE_ADDR    0x0F // Start Rx data
#define REG_FIFO_RX_CURRENT_ADDR 0x10 // Start address of last packet received
#define REG_FIFO_RX_BYTE_ADDR    0x25 // Address of last byte written in FIFO

#define REG_IRQ_FLAGS_MASK 0x11 // Optional IRQ flag mask
#define REG_IRQ_FLAGS      0x12 // IRQ flags
#define REG_RX_NB_BYTES    0x13 // Number of received bytes
#define REG_PKT_RSSI_VALUE 0x1A // RSSI of last packet
#define REG_PKT_SNR_VALUE  0x1B // Current RSSI
#define REG_MODEM_CONFIG_1 0x1D // Modem PHY config 1
#define REG_MODEM_CONFIG_2 0x1E // Modem PHY config 2
#define REG_PREAMBLE_MSB   0x20 // Size of preamble (MSB)
#define REG_PREAMBLE_LSB   0x21 // Size of preamble (LSB)
#define REG_PAYLOAD_LENGTH 0x22 // LoRa TM payload length
#define REG_MODEM_CONFIG_3 0x26 // Modem PHY config 3
#define REG_RSSI_WIDEBAND  0x2C // Wideband RSSI meas-urement

#define REG_DETECT_OPTIMIZE     0x31 // LoRa detection Optimize for SF=6
#define REG_INVERT_IQ           0x33 // Invert LoRa I and Q signals
#define REG_DETECTION_THRESHOLD 0x37 // LoRa detection threshold for SF=6
#define REG_SYNC_WORD           0x39 // LoRa Sync Word

// Modes, REG_OP_MODE register (look `RegOpMode` in datasheeet)
// bits 2-0
#define MODE_SLEEP         0x00 // Sleep
#define MODE_STDBY         0x01 // Standby (default)
#define MODE_FS_TX         0x02 // Frequency Synthesis TX (FSTx)
#define MODE_TX            0x03 // Transmit (Tx)
#define MODE_FS_RX         0x04 // Frequency Synthesis RX (FSRx)
#define MODE_RX_CONTINUOUS 0x05 // Receive (continuous) (Rx)
#define MODE_RX_SINGLE     0x06 // Receive single (RXsingle) [LoRa mode only]
#define MODE_CAD           0x07 // Channel Activity Detection (CAD) [in LoRa mode only]
#define MODES_MASK         0x07 // Modes bit mask 1

// bit 3 (0 -> access to HF registers from 0x61 address, 1 -> access to LF registers)
#define MODE_LOW_FREQ_MODE_ON 0x08 // `LowFrequencyModeOn` 

// bits 6-5 `ModulationType` [FSK/OOK modes only]
#define MODE_FSK    0x00 // 0b00 -> FSK
#define MODE_OOK    0x40 // 0b01 -> OOK
#define MODES_MASK2 0x60 // Modes bit mask 2 

// bit 6 (allows access to FSK registers in 0x0D:0x3F in LoRa mode)
#define MODE_ACCESS_SHARED_REG 0x40 // `AccessSharedReg` (LoRa mode only)

// bit 7 (0 -> FSK/OOK mode, 1 -> LoRa mode)
#define MODE_LONG_RANGE 0x80 // bit 7: `LongRangeMode`

//REG_PA_CONFIG bits (look `RegPaConfig` in datasheet)
#define PA_SELECT 0x80 // bit 7: `PaSelect`

// REG_IRQ_FLAGS (`RegIrqFlags` in datasheet) bits (LoRa)
#define IRQ_TX_DONE           0x08 // `TxDone`
#define IRQ_RX_DONE           0x40 // `RxDone`
#define IRQ_PAYLOAD_CRC_ERROR 0x20 // `PayloadCrcError`

// REG_IRQn_FLAGS (`RegIrqFlagsN` in datasheet) bits (FSK/OOK)
#define IRQ1_RX_READY 0x40 // bit 6: `RxReady`
#define IRQ1_TX_READY 0x20 // bit 5: `TxReady`

#define IRQ2_FIFO_FULL     0x80 // bit 7: `FifoFull`
#define IRQ2_FIFO_EMPTY    0x40 // bit 6: `FifoEmpty`
#define IRQ2_FIFO_LEVEL    0x20 // bit 5: `FifoLevel`
#define IRQ2_FIFO_OVERRUN  0x10 // bit 4: `FifoOverrun`
#define IRQ2_PACKET_SENT   0x08 // bit 3: `PacketSent`
#define IRQ2_PAYLOAD_READY 0x04 // bit 2: `PayloadReady`
#define IRQ2_CRC_OK        0x02 // bit 1: `CrcOk`
#define IRQ2_LOW_BAT       0x01 // bit 0: `LowBat`

// REG_FIFO_THRESH (`RegFifoThresh` in datasheet) bits
#define TX_START_FIFO_LEVEL   0x00 // bit 7: 0 -> `FifoLevel` (use `FifoThreshhold`)
#define TX_START_FIFO_NOEMPTY 0x80 // bit 7: 1 -> `FifoEmpty` (start if FIFO no empty)

// REG_IRQ_FLAGS_MASK (`RegIrqFlagsMask` in datasheet) bits (LoRa)
#define IRQ_RX_DONE_MASK 0x40 // bit 6: `RxDoneMask`

#define FIFO_TX_BASE_ADDR 0x00 // 0x80 FIXME
#define FIFO_RX_BASE_ADDR 0x00 

// Constants
#define FREQ_XTAL  32000000    // 32 MHz
#define FREQ_STEP  61.03515625 // FREQ_XTAL / 2**19 [Hz]

#define FREQ_MAGIC_0 31    // round(FREQ_STEP/2) [Hz]
#define FREQ_MAGIC_1 8     // arithmetic shift
#define FREQ_MAGIC_2 625   // 5**4
#define FREQ_MAGIC_3 25    // 5**2
#define FREQ_MAGIC_4 15625 // 625*25

#define MAX_PKT_LENGTH 255 // maximum packet length [bytes]

// BandWith table [kHz] (LoRa)
#define BW_TABLE {  7800, 10400,  15600,  20800,  31250, \
                   41700, 62500, 125000, 250000, 500000 }

// RX BandWith table (FSK/OOK)
#define RX_BW_TABLE {\
  {2, 7,   2600}, \
  {2, 7,   3100}, \
  {0, 7,   3900}, \
  {2, 6,   5200}, \
  {1, 6,   6300}, \
  {0, 6,   7800}, \
  {2, 5,  10400}, \
  {1, 5,  12500}, \
  {0, 5,  15600}, \
  {2, 4,  20800}, \
  {1, 4,  25000}, \
  {0, 4,  31300}, \
  {2, 3,  31700}, \
  {1, 3,  50000}, \
  {0, 3,  62500}, \
  {2, 2,  83300}, \
  {1, 2, 100000}, \
  {0, 2, 125000}, \
  {2, 1, 166700}, \
  {1, 1, 200000}, \
  {0, 1, 250000}}

//----------------------------------------------------------------------------
#endif // SX127X_DEF_H

/*** end of "sx127x_def.h" file ***/


