/* こーどねーむ「ホンコン」 with Arduino 高速版ファームウェア
Original: Taniyama 2016/2/14 [https://susumutaniyama.github.io/HongKongArduino/]
Modification: RGBA_CRT 2016/3/19 [rgba3crt1p@gmail.com]
Some code is referred on [https://github.com/sanni/cartreader/].
Protocol notes: https://github.com/RGBA-CRT/HongKongArduinoClone/wiki/Firmware-docs
*/
#pragma GCC push_options
#pragma GCC optimize("O3")
//config
//シリアルコンバータがCH340の場合1000000bpsが限界
#define INITIAL_BAUDRATE 115200
#define SERIAL_CONFIG SERIAL_8N1

#define HKAC_DEBUG
#ifndef HKAC_DEBUG
#define FIRMWARE_NAME "HKAF"
#define FIRMWARE_VERSION "5"  // FWのAPIが変わったらインクリメント
#else
#define FIRMWARE_NAME "HKAD"  // debug branch
#define FIRMWARE_VERSION "1"
#endif
const char* FIRMWARE_ID = (FIRMWARE_NAME FIRMWARE_VERSION);
#define BUFFER_LEN 0x400  //ホスト側とサイズを合わせる
#define RX_BUFFER_LEN BUFFER_LEN
static_assert((RX_BUFFER_LEN % 512) == 0, "RX_BUFFER is must be multiple value of page_size");

/* version history
 * HKAF0: 2017/02: add version cmd
 * HKAF1: 2018/02: dynamic baud rate, speedup: 82KB/s
 * HKAF2: 2018/03: protocol change
 * HLAF3: 2018/07: clock cmd change
 * HKAF4: 2029/03: ST017 surpport
 * HKAF5: 2025/08: flash write
 */



//CONFIG
#define _ENABLE_CIC
#define ENABLE_ST018_BIOS_DUMP
#define ENABLLE_SFMEM
// #define BUILD_SUPER_SLOW_READ

//----------------- 実験コード ------------------
#ifdef _ENABLE_CIC

#include "si5351.h"
#include "Wire.h"
#include <avr/io.h>

//I2C通信状態を解除してA4,A5ピンを使用可能に
#define DISABLE_I2C() \
  { TWCR = 0; }
#define ENABLE_I2C() \
  { TWCR = 0x45; }

//クロックジェネレータ
Si5351 clockgen;
#endif
//--------------------------------------------

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
// #define SWAP_CEOE
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
#if 0
#define CART_OUTPUT_ENABLE() PORTC &= ~(PIN_PORTC_OE_MASK)
#define CART_OUTPUT_DISABLE() PORTC |= PIN_PORTC_OE_MASK
#define CART_OUTPUT_TOGGLE() PINC = PIN_PORTC_OE_MASK
#else
#define CART_OUTPUT_ENABLE() PORTC &= val_oe_and_mask
#define CART_OUTPUT_DISABLE() PORTC |= val_oe_or_mask
#define CART_OUTPUT_TOGGLE() PINC = val_oe_or_mask

#define CART_CHIP_ENABLE() PORTC &= ~val_ce_or_mask
#define CART_CHIP_DISABLE() PORTC |= val_ce_or_mask
#define CART_CHIP_TOGGLE() PINC = val_ce_or_mask
#endif

// databus
#define getDataPin() (PIND >> 2) | ((PINB << 6))

#define Serial_readWord() ((word)Serial.read() | ((word)Serial.read() << 8))

// RXバッファ
byte buf[BUFFER_LEN];

//現在のアドレスの状態
byte lastadr[3];

byte gflags;
#define FLASH_CONFIG_CEOE_SWAP 0x01
#define FLASH_CONFIG_BYTE_VERIFY 0x02
#define GFLAGS_SUPER_SLOW_READ 0x04
#define FLASH_CONFIG_VERIFY_FIXED_VALUE 0x08

//-----------------
// serial comm
//-----------------

inline void serial_send(byte data) {
  while (!(UCSR0A & _BV(UDRE0)))
    ;  //UDRが空になるのを待つ
  UDR0 = data;
}

// recive to buffer
void hostsync_receive(word length) {
  noInterrupts();
  //Send 'R'equest Signal
  serial_send('R');
  word i = length;
  word o = 0;
  do {
    while (!(UCSR0A & _BV(RXC0)))
      ;
    buf[o++] = UDR0;
  } while (--i);
  interrupts();
}

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



//--------------
// snes level
//--------------
#define LO_TO_REAL_ADDRESS(bank, address) \
  { \
    bank = (bank << 1) | (address >> 15); \
    address |= 0x8000; \
  }
//アドレスバスを設定
inline void setAddress_(byte bank, word address) {
  setFF(0, address);
  setFF(1, address >> 8);
  setFF(2, bank);
}

inline void setAddress(byte bank, word address, byte isLoROM) {
  if (isLoROM) {
    LO_TO_REAL_ADDRESS(bank, address);
  }

  BB_OUT_DISABLE();
  setAddress_(bank, address);
  // longWait();
}

void readCart(byte isLoROM) {
  while (Serial.available() < 5)
    ;
  word address = Serial_readWord();
  byte bank = Serial.read();
  word datasize = Serial_readWord();
  BB_DIR_INPUT();

  // note: 一見最適化の余地があるが、結局serial_sendで待ちが発生するので意味無し
  do {
    setAddress(bank, address++, isLoROM);
    // SA1でおかしくなったらsetAddressにnopを仕込む

    serial_send(readData());
  } while (--datasize);
}

void writeCart(int isLoROM = false) {
  //コマンド受信
  while (Serial.available() < 5)
    ;
  word address = Serial_readWord();
  byte bank = Serial.read();
  word datasize = Serial_readWord();

  CART_WRITE_DISABLE();
  CART_OUTPUT_DISABLE();
  BB_DIR_OUTPUT();

  word goalAdr = address + datasize;
  word bufpos = RX_BUFFER_LEN;  //buffer ptr, 最初は必ず受信させる

  while (1) {
    //データ受信
    if (bufpos >= RX_BUFFER_LEN) {
      hostsync_receive(RX_BUFFER_LEN);
      bufpos = 0;
    }

    writebyte_cart(bank, address, buf[bufpos]);
    address++;
    bufpos++;

    if (address == goalAdr) break;
  }

  CART_WRITE_DISABLE();
  BB_OUT_DISABLE();
  BB_DIR_INPUT();

  //Send End Signal
  serial_send('E');
  //  Serial.println("WRITE_END");
}

/*
  inline byte hex2ascii(byte hex) {
  return (hex < 0xA) ? hex + '0' : hex - 0xA + 'A';
  }

  void send_hexdump(byte hex) {
  Serial.write(hex2ascii((hex >> 4) & 0x0f));
  Serial.write(hex2ascii(hex & 0x0f));
  }*/


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

void longWait() {
  __asm__ volatile("nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t");  // nop*16
                                // 1us～1.6usぐらい
  // __asm__ volatile("nop\n\t"
  //                  "nop\n\t"
  //                  "nop\n\t"
  //                  "nop\n\t");  // nop*4
  //                               // for MX29L3211MC
}

byte readData() {
  CART_OUTPUT_ENABLE();
  dataPinDirInput();
  BB_OUT_ENABLE();

  // AddressOutputDelay: 110nsぐらい
  // Output Enable to Output Delay: 25nsぐらい
  // Arduino Uno@16MHzでToggle nop2回 Toggle で200nsぐらい。
  // 立ち上がり直前でラッチしたいので、前準備に時間かけていいけどRead後は小さくすると良い
  longWait();
#ifdef BUILD_SUPER_SLOW_READ
  byte b;
  if (gflags & GFLAGS_SUPER_SLOW_READ) {
    const byte ok_retry_max = 10;
    const byte ng_retry_max = 30;
    byte ok_cnt = ok_retry_max;
    byte ng_cnt = ng_retry_max;
    byte err = 0, b2;
retry:
    b = getDataPin();
    longWait();
    b2 = getDataPin();
    if (b2 == b) {
      ng_cnt = ng_retry_max;
      if (--ok_cnt) { goto retry; }
    } else {
      ok_cnt = ok_retry_max;
      err++;
      if (--ng_cnt) { goto retry; }
    }
    // b ^= b2;// error bit report
    // b = err; // error count report
    b = b2;
  } else {
    b = getDataPin();
  }
#else
  byte b = getDataPin();
#endif

  CART_OUTPUT_DISABLE();

  BB_OUT_DISABLE();
  dataPinDirOutput();
  return b;
}




// /REを制御して読み込む
inline byte readbyte_cart(byte bank, word address) {
  setAddress(bank, address, false);

  // /OEのパルスを成立させるためのWait
  __asm__ volatile("nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t");
  //__asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");

  CART_OUTPUT_ENABLE();

  // NOPの数検証済み 4LINE（SA1のSRAM WRITE VERIFY）
  __asm__ volatile("nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t");

  byte ret = readData();

  CART_OUTPUT_DISABLE();
  return ret;
}

//　/WRとかをちゃんと制御して書き込む
void writebyte_cart(byte bank, word address, byte data) {
  setAddress(bank, address, false);
  setDataPin(data);

  BB_DIR_OUTPUT();
  BB_OUT_ENABLE();

  // /WEパルス成立 & 74HC245 -> SFC へのデータ安定化のWAIT
  // 74HC245 -> SFC へのデータ安定化
  __asm__ volatile("nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t");

  CART_WRITE_ENABLE();

  // /WEパルスの時間稼ぎ
  // SA1のSRAM Writeではこの5行分の長さが必要(TESTED: SA1 SRAM WRITE)
  __asm__ volatile("nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t");

  CART_WRITE_DISABLE();

  BB_OUT_DISABLE();
  BB_DIR_INPUT();
}

#ifndef _ENABLE_CIC

byte haveClockModule = 0;
#endif
#ifdef _ENABLE_CIC
byte haveClockModule = 0;
//[Nintendo Cart Reader]より
void setupCloclGen(bool clk1_en, bool clk2_en, bool clk3_en, bool clk2_oc) {
  // Adafruit Clock Generator
  ENABLE_I2C();
  haveClockModule = clockgen.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  /*  clockgen.pll_reset(SI5351_PLLA);
    clockgen.pll_reset(SI5351_PLLB);*/
  if (haveClockModule) {
    clockgen.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
    clockgen.set_pll(SI5351_PLL_FIXED, SI5351_PLLB);
    clockgen.set_freq(2147727200ULL, SI5351_CLK0);
    if (clk2_oc) {
      //over clock for SA-1
      clockgen.set_freq(659000000ULL, SI5351_CLK1);
    } else {
      //normal clock
      clockgen.set_freq(357954500ULL, SI5351_CLK1);
    }
    clockgen.set_freq(307200000ULL, SI5351_CLK2);
    clockgen.output_enable(SI5351_CLK0, clk1_en);
    clockgen.output_enable(SI5351_CLK1, clk2_en);
    clockgen.output_enable(SI5351_CLK2, clk3_en);
    /*   clockgen.set_clock_invert(SI5351_CLK0, 1);
       clockgen.set_clock_invert(SI5351_CLK1, 1);
       clockgen.set_clock_invert(SI5351_CLK2, 1);*/
  }
  DISABLE_I2C();
}
#endif

void setup() {
  //コントロールピンをすべてOUTPUTに
  for (int i = GD; i <= RST; i++)
    pinMode(i, OUTPUT);

  digitalWrite(GD, LOW);   //GD(PORTB 02) PWM DISABLE
  digitalWrite(GD, HIGH);  // Disable
  digitalWrite(G0, HIGH);  // Disable
  digitalWrite(G1, HIGH);  // Disable
  digitalWrite(G2, HIGH);  // Disable

  digitalWrite(CK, LOW);

  digitalWrite(DIR, LOW);  // 入力

  digitalWrite(OE, HIGH);
  digitalWrite(CS, HIGH);
  digitalWrite(WE, HIGH);
  digitalWrite(RST, LOW);

#ifdef _ENABLE_CIC
  //クロックジェネレータの動作を開始
  setupCloclGen(true, false, true, false);
#endif

  SetFlashOECtrl(false);
  //アクセスランプを消灯＆アドレスバス初期化
  //setAddressは差分しかセットしないので、手動でFlipFlopを初期化
  dataPinDirInput();
  BB_OUT_DISABLE();
  BB_DIR_INPUT();
  setFF(0, 0x00);
  setFF(1, 0x00);
  setFF(2, 0x00);

  digitalWrite(OE, HIGH);
  digitalWrite(CS, LOW);
  digitalWrite(WE, HIGH);
  digitalWrite(RST, HIGH);

  // Pull-Donw disable
  MCUCR |= 0x10;

  Serial.begin(INITIAL_BAUDRATE, SERIAL_CONFIG);
}

void loop() {
  while (Serial.available() == 0)
    ;  //wait command
  byte cmd = Serial.read();

  switch (cmd) {
    case 'R':
      {
        readCart(false);
      }
      break;
    case 'r':
      {
        readCart(true);
      }
      break;

    case 'd':
      {
        serial_send(readData());
      }
      break;

    case 'a':
    case 'A':
      {  //Set address
        while (Serial.available() < 3)
          ;
        byte isLoROM = (cmd == 'a');
        word address = Serial_readWord();
        byte bank = Serial.read();
        setAddress(bank, address, isLoROM);
      }
      break;

    case 'b':
      {  //Set boudrate
        while (Serial.available() < 4)
          ;

        unsigned long new_boudrate = Serial.read()
                                     | (unsigned long)Serial.read() << 8
                                     | (unsigned long)Serial.read() << 16
                                     | (unsigned long)Serial.read() << 24;

        Serial.end();
        Serial.begin(new_boudrate);
        //Serial.flush();
        //このあと、ファームチェックで値が正常に帰ってくることを確認してから
        //各種コマンドを投げてください
        setAddress(0xFF, 0xFFFF, false);
      }
      break;

    case 'c':
      {  //set control bus(OE WD RST CS)
        while (Serial.available() < 1)
          ;
        setCtrlBus(Serial.read());
      }
      break;

    case 'f':
      {  // flash command
        flashWriteCart();
      }
      break;

    case 'F':
      {  // flash config
        flashReceiveConfig();
      }
      break;

    case 'g':
      {  //CPU ClockGen Start/Stop
        while (Serial.available() < 1)
          ;
        byte mode = Serial.read();
#ifdef _ENABLE_CIC
        if ((mode & 0xf0) == 0x30) {
          setupCloclGen(mode & 0x01, mode & 0x02, mode & 0x04, mode & 0x08);
        } else {
          setupCloclGen(true, false, true, false);
        }
#endif
      }  //続いてステータスの返却へ

    case 'G':
      {
        //return clock module status
        Serial.write('0' | haveClockModule);
      }
      break;

    case 'i':
      {  // print infomation
        Serial.write(FIRMWARE_ID);
        Serial.write((char)FIRMWARE_VERSION);
#ifdef _ENABLE_CIC
        Serial.print("-CIC ");
        Serial.print(haveClockModule ? "[con]" : "[dis]");
#endif
        // Serial.print("\nABus:");
        // Serial.print(lastadr[2], HEX);
        // Serial.print(lastadr[1], HEX);
        // Serial.print(lastadr[0], HEX);
        // Serial.print("\nFash:\n");
        // Serial.print(flash_bank, HEX);        Serial.print(flash_address[0], HEX);        Serial.print(":");        Serial.print(flash_cmd[0], HEX);        Serial.print(",\t");
        // Serial.print(flash_bank, HEX);        Serial.print(flash_address[1], HEX);        Serial.print(":");        Serial.print(flash_cmd[1], HEX);        Serial.print(",\t");
        // Serial.print(flash_bank, HEX);        Serial.print(flash_address[2], HEX);        Serial.print("\n");
        // for (byte i = 0; i < 20; i++) {
        //   Serial.print((char)readbyte_cart(0x00, 0xffc0 + i));
        // }  Serial.print("\n");
      }
      break;

    case 's':
      {  // set register(1byte write)
        Serial.print((char)readbyte_cart(0xc0, 0x0000));
        writebyte_cart(0x00, 0x2220, 04);
        Serial.print((char)readbyte_cart(0xc0, 0x0000));
      }
      break;

#ifdef ENABLLE_SFMEM
    case 'S':
      {  // setup SF memory
        digitalWrite(OE, HIGH);
        digitalWrite(CS, LOW);
        digitalWrite(WE, HIGH);
        digitalWrite(RST, HIGH);

        writebyte_cart(0x00, 0x2400, 0x09);
        byte status = readbyte_cart(0x00, 0x2400);
        writebyte_cart(0x00, 0x2401, 0x28);
        writebyte_cart(0x00, 0x2401, 0x84);
        writebyte_cart(0x00, 0x2400, 0x06);
        writebyte_cart(0x00, 0x2400, 0x39);

        if (readbyte_cart(0x00, 0x2400) == 0x2A)
          Serial.print("OK");
        else
          Serial.print("NG");
        Serial.write(readbyte_cart(0x00, 0x2400));
      }
      break;
#endif

    case 'T':
    case 't':
      {  // set register(1byte write)
        while (Serial.available() < 4)
          ;
        byte bank = Serial.read();
        word address = Serial_readWord();
        byte data = Serial.read();

        //lorom -> real address
        if (cmd == 't') {
          //LO_TO_REAL_ADDRESS(bank, address);
        }
        //        digitalWrite(CS, HIGH);
        //        CART_OUTPUT_DISABLE();
        //        CART_WRITE_DISABLE();
        writebyte_cart(bank, address, data);
      }
      break;

    case 'W':
    case 'w':
      {  //bulk write cart
        byte isLoROM = (cmd == 'w');
        writeCart(isLoROM);
      }
      break;

    case 'v':
      {  //Return fimware version
        Serial.write(FIRMWARE_ID);
        setAddress(0x00, lastadr[1] << 8, false);
      }
      break;

#ifdef ENABLE_ST018_BIOS_DUMP
    case 'z':
    case 'Z':
      {
        st018_biosDump(cmd);
      }
      break;
#endif
    default:
      Serial.write("?INVALIDCMD=");
      Serial.write(cmd);
  }
}

#pragma GCC pop_options
