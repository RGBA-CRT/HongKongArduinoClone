#ifndef __SNES_IO_H__
#define __SNES_IO_H__
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
static uint8_t val_oe_or_mask;  // speed > ram_usage
static uint8_t val_oe_and_mask;
static uint8_t val_ce_or_mask;  // speed < ram_usage

void SetFlashOECtrl(bool swap_ce_oe) {
  if (!swap_ce_oe) {
    val_oe_or_mask = PIN_PORTC_OE_MASK;
    val_oe_and_mask = ~(PIN_PORTC_OE_MASK);
    val_ce_or_mask = PIN_PORTC_CE_MASK;
  } else {
    val_oe_or_mask = PIN_PORTC_CE_MASK;
    val_oe_and_mask = ~PIN_PORTC_CE_MASK;
    val_ce_or_mask = PIN_PORTC_OE_MASK;
  }
}

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

//データーバスへ値をセット
inline void setDataPin(byte b) {
#if 0
  PORTD &= 0b00000011;  //CLEAR
  PORTD |= b << 2;      //ORでセット
#else
  PORTD = b << 2;  // direct set test
#endif
  PORTB &= 0b11111100;
  PORTB |= b >> 6;
}

//アドレスバスを構成するFlip-Flopへ値をセット
inline void setFF(byte ch, byte b) {
  //digitalWrite(G0 + ch, LOW); // FF番号chをWriteEnableに
  PINB = (0b00001000 << ch);
  setDataPin(b);

  // digitalWrite(CK, HIGH);
  PINC = 0b00000010;

  //digitalWrite(CK, LOW);
  PINC = 0b00000010;

  //digitalWrite(G0 + ch, HIGH); // WriteDisable
  PINB = (0b00001000 << ch);
}

//アドレスバスを設定
inline void setAddressFFs(byte bank, word address) {
  setFF(0, address);
  setFF(1, address >> 8);
  setFF(2, bank);
}


void setCtrlBus(byte b) {
  if (b & 0b0001) {
    CART_OUTPUT_DISABLE();
  } else {
    CART_OUTPUT_ENABLE();
  }
  if (b & 0b0010) {
    CART_CHIP_DISABLE();
  } else {
    CART_CHIP_ENABLE();
  }
  digitalWrite(WE, (b & 0b0100) ? HIGH : LOW);
  digitalWrite(RST, (b & 0b1000) ? HIGH : LOW);
}

#endif