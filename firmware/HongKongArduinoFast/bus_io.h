#ifndef __SNES_IO_H__
#define __SNES_IO_H__

#pragma GCC push_options
#pragma GCC optimize("Ofast")

//データバス[PORTD]
#define DATA0 2
#define DATA1 3
#define DATA2 4
#define DATA3 5
#define DATA4 6
#define DATA5 7

//[PORTB]
#define DATA6 8
#define DATA7 9

//74HCシリーズの制御
#define GD 10
#define G0 11
#define G1 12
#define G2 13

//[PORTC]コントロールピン
#define DIR 14
#define CK 15
#define OE 16
#define CS 17
#define WE 18
#define RST 19

const uint8_t PIN_PORTC_OE_MASK = 0b00000100;
const uint8_t PIN_PORTC_CE_MASK = 0b00001000;

//bus buffer direction
#define BB_DIR_OUTPUT() PORTC |= 0x01  // DIR=HIGH
#define BB_DIR_INPUT() PORTC &= 0xfe   // DIR=LOW
#define BB_DIR_TOGGLE() PINC = 0x01

//bus buffer OutputControl
#define BB_OUT_DISABLE() PORTB |= 0b00000100
#define BB_OUT_ENABLE() PORTB &= 0b11111011
#define BB_OUT_TOGGLE() PINB = 0b00000100

// cart /WE control
#define CART_WRITE_ENABLE() PORTC &= 0b11101111
#define CART_WRITE_DISABLE() PORTC |= 0b00010000
#define CART_WRITE_TOGGLE() PINC = 0b00010000

// cart /OE control
uint8_t val_oe_or_mask;  // speed > ram_usage
uint8_t val_oe_and_mask;
uint8_t val_ce_or_mask;  // speed < ram_usage

#define CART_OUTPUT_ENABLE() PORTC &= val_oe_and_mask
#define CART_OUTPUT_DISABLE() PORTC |= val_oe_or_mask
#define CART_OUTPUT_TOGGLE() PINC = val_oe_or_mask

#define CART_CHIP_ENABLE() PORTC &= ~val_ce_or_mask
#define CART_CHIP_DISABLE() PORTC |= val_ce_or_mask
#define CART_CHIP_TOGGLE() PINC = val_ce_or_mask

// databus
#define getDataPin() (PIND >> 2) | ((PINB << 6))

//--------------
//   ic level
//--------------

//データピンの方向設定
#define dataPinDirInput() \
  do { \
    DDRD &= 0b00000011; \
    DDRB &= 0b11111100; \
  } while (0);

#define dataPinDirOutput() \
  do { \
    DDRD |= 0b11111100; \
    DDRB |= 0b00000011; \
  } while (0);

void setDataPin(byte b);
void setFF(byte ch, byte b);
void setAddressFFs(byte bank, word address);
void setCtrlBus(byte b);

void inline SetFlashOECtrl(bool swap_ce_oe);
#pragma GCC pop_options

#endif