/* こーどねーむ「ホンコン」 with Arduino 高速版ファームウェア
   Original : たにやま 2016/2/14 [http://hongkongarduino.web.fc2.com/]
   Optimize : RGBA_CRT 2016/3/19 [rgba3crt1p@gmail.com]
   Some codes are referenced to [https://github.com/sanni/cartreader/]
   Docs : https://github.com/RGBA-CRT/HongKongArduinoClone/wiki/Firmware-docs
*/

//config
//シリアルコンバータがCH340の場合1000000bpsが限界
#define INITIAL_BAUDRATE 115200
#define SERIAL_CONFIG SERIAL_8N1
#define FIRMWARE_ID "HKAF"
#define FIRMWARE_VERSION '4' // FWのAPIが変わったらインクリメント

//クロック回路有効
#define _ENABLE_CIC

//----------------- 実験コード ------------------
#ifdef _ENABLE_CIC

#include "si5351.h"
#include "Wire.h"
#include <avr/io.h>

//I2C通信状態を解除してA4,A5ピンを使用可能に
#define DISABLE_I2C() { TWCR = 0;}
#define ENABLE_I2C() {TWCR = 0x45;}

//クロックジェネレータ
Si5351 clockgen;
#endif
//--------------------------------------------

//データバス[PORTD]
#define DATA0  2
#define DATA1  3
#define DATA2  4
#define DATA3  5
#define DATA4  6
#define DATA5  7

//[PORTB]
#define DATA6  8
#define DATA7  9

//74HCシリーズの制御
#define GD  10
#define G0  11
#define G1  12
#define G2  13

//[PORTC]コントロールピン
#define DIR  14
#define CK  15
#define OE  16
#define CS  17
#define WE  18
#define RST  19


//bus buffer direction
#define BB_DIR_OUTPUT() PORTC |= 0x01 // DIR=HIGH
#define BB_DIR_INPUT() PORTC &= 0xfe  // DIR=LOW

//bus buffer OutputControl
#define BB_OUT_DISABLE() PORTB |= 0b00000100
#define BB_OUT_ENABLE()   {volatile uint8_t oldSREG = SREG;cli();PORTB &= 0b11111011;SREG = oldSREG;}

// cart /WE control
#define CART_WRITE_ENABLE()   PORTC &= 0b11101111
#define CART_WRITE_DISABLE()  PORTC |= 0b00010000

// cart /OE control
#define CART_OUTPUT_ENABLE()   PORTC &= 0b11111011
#define CART_OUTPUT_DISABLE()  PORTC |= 0b00000100


#define Serial_readWord() ((word)Serial.read() | ((word)Serial.read() << 8))

//バッファ
#define BUFFER_LEN 0xFF //ホスト側とサイズを合わせる
#define RX_BUFFER_LEN BUFFER_LEN
byte buf[BUFFER_LEN];

//現在のアドレスの状態
byte lastadr[3];

//-----------------
// serial comm
//-----------------

inline void serial_send(byte data) {
  while ( !(UCSR0A & _BV(UDRE0)) ); //UDRが空になるのを待つ
  UDR0 = data;
}

// recive to buffer
inline void serial_receive(byte length) {
  byte i = 0;
  while (i < length) {
    while (Serial.available() < 1);
    buf[i] = Serial.read();
    i++;
  }
}

//--------------
//   ic level
//--------------

//データピンの方向設定
inline void setDataDir(byte DATADIR)
{
  if (DATADIR == INPUT) {
    DDRD &= 0b00000011;
    DDRB &= 0b11111100;
  } else {
    DDRD |= 0b11111100;
    DDRB |= 0b00000011;
  }
}

//データーバスへ値をセット
inline void setData(byte b)
{
  PORTD &= 0b00000011;  //CLEAR
  PORTD |= b << 2;      //ORでセット
  PORTB &= 0b11111100;
  PORTB |= b >> 6;
}

//アドレスバスを構成するFlip-Flopへ値をセット
inline void setFF(byte ch, byte b)
{
  //digitalWrite(G0 + ch, LOW); // FF番号chをWriteEnableに
  PORTB &= ~0b00001000 << ch;
  setData(b);

  // digitalWrite(CK, HIGH);
  PORTC |= 0b00000010;

  //digitalWrite(CK, LOW);
  PORTC &= 0b11111101;

  //digitalWrite(G0 + ch, HIGH); // WriteDisable
  PORTB |= 0b00001000 << ch;
}



//--------------
// snes level
//--------------
#define LO_TO_REAL_ADDRESS(bank,address) {bank = (bank << 1) | (address >> 15);  address |= 0x8000;}
//アドレスバスを設定
inline void setAddress(byte bank, word address, byte isLoROM)
{
  if (isLoROM) {
    LO_TO_REAL_ADDRESS(bank, address);
  }

  BB_OUT_DISABLE();

  //変更のないFlipFlopはいじらない
  byte spritAdr = address;
  if (lastadr[0] != spritAdr) {
    setFF(0, spritAdr);
    lastadr[0] = spritAdr;
  }

  spritAdr = address >> 8;
  if (lastadr[1] != spritAdr) {
    setFF(1, spritAdr);
    lastadr[1] = spritAdr;
  }

  if (lastadr[2] != bank) {
    setFF(2, bank);
    lastadr[2] = bank;
  }
}


inline void readCart(byte isLoROM) {
  while (Serial.available() < 5);
  word address = Serial_readWord();
  byte bank = Serial.read();

  word datasize = Serial_readWord();

  word goalAdr = address + datasize;
  while (1) {
    // CART_OUTPUT_ENABLE();
    setAddress(bank, address, isLoROM);
    __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
    serial_send(readData());
    // CART_OUTPUT_DISABLE();
    address++;
    if (address == goalAdr) break; //00:C000-01:0000
  }

}

void writeCart(int isLoROM = false) {
  //コマンド受信
  while (Serial.available() < 5);
  word address = Serial_readWord();
  byte bank = Serial.read();
  word datasize =  Serial_readWord();

  CART_WRITE_DISABLE();
  CART_OUTPUT_DISABLE();
  BB_DIR_OUTPUT();

  word goalAdr = address + datasize;
  byte bufpos = RX_BUFFER_LEN; //buffer ptr, 最初は必ず受信させる

  while (1) {
    //データ受信
    if (bufpos >= RX_BUFFER_LEN) {
      //Send 'R'equest Signal
      serial_send('R');
      serial_receive(RX_BUFFER_LEN);
      bufpos = 0;
    }

    writebyte_cart(bank, address, buf[bufpos]);
    address++; bufpos++;

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

void st018_command(byte cmd) {
  //wait for transferReady
  st018_transferWait();

  //cmd
  writebyte_cart(0x00, 0x3802, cmd);

  //wait
  st018_transferWait();

  return;
}

inline byte st018_readData() {
  if (st018_readyWait())
    return 'E';
  return st018_readbyte_cart(0x00, 0x3800);
}

// タイムアウトつけてないけど大丈夫か
inline void st018_transferWait() {
  while ((st018_readbyte_cart(0x00, 0x3804) & 0x10)) ;
}

bool st018_readyWait() {
  byte waitCount = 1;

  // STATUSのReadyが1になるか、waitCountで255ループするまで待つ
  //                                                    ↓ &&ではない（可読性のないコード）
  while (((st018_readbyte_cart(0x00, 0x3804) & 0x01) != 0x01) & waitCount) {
    waitCount++;
    delayMicroseconds(10);
  }
  return (waitCount == 0);
}

// err=true
bool st018_reset() {
  // reset
  writebyte_cart(0x00, 0x3804, 0x00);  delayMicroseconds(100);
  writebyte_cart(0x00, 0x3804, 0xff);  delayMicroseconds(100);
  writebyte_cart(0x00, 0x3804, 0x00);

  //STATが0以外になるまで待つ
  byte waitCount = 0;
  while (st018_readbyte_cart(0x00, 0x3804) == 0x00) {
    if (waitCount > 250)
      return true;
    waitCount++;
    delayMicroseconds(8000);
  }
  
  return false;
}

void st018_memread(byte cmd, byte n_kb) {
  st018_command(cmd);

  // 1バイト読み捨て
  st018_readData();
  
  for (byte j = 0; j < n_kb; j++) {
    for (word i = 0; i < 1024; i++) {
      //serial_send(st018_readData());
      Serial.write(st018_readData());
    }

    // 1KB毎に指示を待つ
    while (Serial.available() < 1);

    // 'e' = Cancel
    if (Serial.read() == 'e')
      break;

  }
}

bool st018_biosDump(byte cmd) {
  // ROMと違ってI/OポートなのでCSは非アクティブである必要がある
  digitalWrite(CS, HIGH);
  
  // reset
  if (st018_reset()) {
    Serial.print("RST ERR");
    return true;
  }

  Serial.write('S');

  // dump program ROM (128KB)
  st018_memread(0xF3, 128);

  // dump data ROM (32KB)
  st018_memread(0xF4, 32);

  // reset
  st018_reset();

  return 0;
}

inline void setCtrlBus(byte b) {
  //  digitalWrite(OE , (b & 0b0001) ? HIGH : LOW);
  digitalWrite(CS , (b & 0b0010) ? HIGH : LOW);
  //  digitalWrite(WE , (b & 0b0100) ? HIGH : LOW);
  digitalWrite(RST, (b & 0b1000) ? HIGH : LOW);
}

inline byte readData()
{
  CART_OUTPUT_ENABLE();
  setDataDir(INPUT);
  BB_OUT_ENABLE();
  __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");

  byte b = (PIND >> 2) | ((PINB << 6));

  BB_OUT_DISABLE();
  setDataDir(OUTPUT);
  CART_OUTPUT_DISABLE();

  return b;
}




// /REを制御して読み込む
inline byte readbyte_cart(byte bank, word address) {
  setAddress(bank, address, false);

  // /OEのパルスを成立させるためのWait
  __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t");
  //__asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");

  CART_OUTPUT_ENABLE();

  // NOPの数検証済み 4LINE（SA1のSRAM WRITE VERIFY）
  __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");

  byte ret = readData();

  CART_OUTPUT_DISABLE();
  return ret;
}


inline byte st018_readbyte_cart(byte bank, word address) {
  setAddress(bank, address, false);

  __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");

  CART_OUTPUT_ENABLE();

  __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");

  __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");

  byte ret = readData();

  CART_OUTPUT_DISABLE();
  return ret;
}

//　/WRとかをちゃんと制御して書き込む
void writebyte_cart(byte bank, word address, byte data) {
  setAddress(bank, address, false);
  setData(data);

  BB_DIR_OUTPUT();
  BB_OUT_ENABLE();

  // /WEパルス成立 & 74HC245 -> SFC へのデータ安定化のWAIT
  // 74HC245 -> SFC へのデータ安定化
  __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");

  CART_WRITE_ENABLE();

  // /WEパルスの時間稼ぎ
  // SA1のSRAM Writeではこの5行分の長さが必要(TESTED: SA1 SRAM WRITE)
  __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");

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

void setup()
{


  //コントロールピンをすべてOUTPUTに
  for (int i = GD; i <= RST; i++)
    pinMode(i, OUTPUT);

  digitalWrite(GD, LOW);  //GD(PORTB 02) PWM DISABLE
  digitalWrite(GD, HIGH); // Disable
  digitalWrite(G0, HIGH); // Disable
  digitalWrite(G1, HIGH); // Disable
  digitalWrite(G2, HIGH); // Disable

  digitalWrite(CK, LOW);

  digitalWrite(DIR, LOW); // 入力

  digitalWrite(OE, HIGH);
  digitalWrite(CS, HIGH);
  digitalWrite(WE, HIGH);
  digitalWrite(RST, LOW);


  //アクセスランプを消灯＆アドレスバス初期化
  //setAddressは差分しかセットしないので、手動でFlipFlopを初期化
  setDataDir(OUTPUT);
  BB_OUT_DISABLE();
  setFF(0, 0x00); lastadr[0] = 0;
  setFF(1, 0x00); lastadr[1] = 0;
  setFF(2, 0x00); lastadr[2] = 0;


#ifdef _ENABLE_CIC
  //クロックジェネレータの動作を開始
  setupCloclGen(true, false, true, false);
#endif

  digitalWrite(OE, HIGH);
  digitalWrite(CS, LOW);
  digitalWrite(WE, HIGH);
  digitalWrite(RST, HIGH);

  Serial.begin(INITIAL_BAUDRATE, SERIAL_CONFIG);
}


//flash config
#define FLASH_COMMAND_LENGTH 3
byte flash_bank;
word flash_address[FLASH_COMMAND_LENGTH]; // = {0xAAAA,0x5555,0xAAAA}
byte flash_cmd[FLASH_COMMAND_LENGTH];     // = {0xAA  ,0x55,  [cmd] }
// flash_cmdの最後のバイトは適宜コマンドに置き換え

void loop() {
  while (Serial.available() == 0);  //wait command
  byte cmd = Serial.read();

  switch (cmd) {
    case 'R':
    case 'r':
      readCart((cmd == 'r'));
      break;

    case 'a':
    case 'A':
      { //Set address
        while (Serial.available() < 3)  ;
        byte isLoROM = (cmd == 'a');
        word address = Serial_readWord();
        byte bank = Serial.read();
        setAddress(bank, address, isLoROM);
      } break;

    case 'b':
      { //Set boudrate
        while (Serial.available() < 4);

        unsigned long new_boudrate = Serial.read()
                                     | (unsigned long)Serial.read() << 8
                                     | (unsigned long)Serial.read() << 16
                                     | (unsigned long)Serial.read() << 24;
        Serial.end();
        Serial.begin(new_boudrate);
        //このあと、ファームチェックで値が正常に帰ってくることを確認してから
        //各種コマンドを投げてください
      } break;

    case 'c':
      { //set control bus(OE WD RST CS)
        while (Serial.available() < 1) ;
        setCtrlBus(Serial.read());
      } break;

    case 'f':
      { // flash command
        while (Serial.available() < 1);
        flash_cmd[FLASH_COMMAND_LENGTH] = Serial.read();
        for (byte i = 0; i < FLASH_COMMAND_LENGTH; i++) {
          writebyte_cart(flash_bank, flash_address[i], flash_cmd[i]);
        }
      } break;

    case 'F':
      { // flash config
        // flash_bank + (flash_address,flash_cmd) * FLASH_COMMAND_LENGTH
        while (Serial.available() < (1 + FLASH_COMMAND_LENGTH * 3));
        flash_bank = Serial.read();
        for (byte i = 0; i < FLASH_COMMAND_LENGTH; i++) {
          flash_address[i] = Serial_readWord();
          flash_cmd[i] = Serial.read();
        }
      } break;

    case 'g':
      { //CPU ClockGen Start/Stop
        while (Serial.available() < 1);
        byte mode = Serial.read();
#ifdef _ENABLE_CIC
        if ((mode & 0xf0) == 0x30) {
          setupCloclGen(mode & 0x01, mode & 0x02, mode & 0x04, mode & 0x08);
        } else {
          setupCloclGen(true, false, true, false);
        }
#endif
      } //続いてステータスの返却へ

    case 'G': {
        //return clock module status
        Serial.write('0' | haveClockModule);
      } break;

    case 'i':
      { // print infomation
        Serial.write(FIRMWARE_ID);
        Serial.write((char)FIRMWARE_VERSION);
#ifdef _ENABLE_CIC
        Serial.print("-CIC ");
        Serial.print(haveClockModule ? "[con]" : "[dis]");
#endif
        Serial.print("\nABus:");
        Serial.print(lastadr[2], HEX);
        Serial.print(lastadr[1], HEX);
        Serial.print(lastadr[0], HEX);
        Serial.print("\nFash:\n");
        Serial.print(flash_bank, HEX);        Serial.print(flash_address[0], HEX);        Serial.print(":");        Serial.print(flash_cmd[0], HEX);        Serial.print(",\t");
        Serial.print(flash_bank, HEX);        Serial.print(flash_address[1], HEX);        Serial.print(":");        Serial.print(flash_cmd[1], HEX);        Serial.print(",\t");
        Serial.print(flash_bank, HEX);        Serial.print(flash_address[2], HEX);        Serial.print("\n");
        for (byte i = 0; i < 20; i++) {
          Serial.print((char)readbyte_cart(0x00, 0xffc0 + i));
        }  Serial.print("\n");

      } break;

    case 's':
      { // set register(1byte write)
        Serial.print((char)readbyte_cart(0xc0, 0x0000));
        writebyte_cart(0x00, 0x2220, 04);
        Serial.print((char)readbyte_cart(0xc0, 0x0000));
      } break;

    case 'S':
      { // setup SF memory
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

        if ( readbyte_cart(0x00, 0x2400) == 0x2A)
          Serial.print("OK");
        else
          Serial.print("NG");
        Serial.write(readbyte_cart(0x00, 0x2400));
      } break;

    case 'T':
    case 't':
      { // set register(1byte write)
        while (Serial.available() < 4);
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
      } break;

    case 'W':
    case 'w':
      { //bulk write cart
        byte isLoROM = (cmd == 'w');
        writeCart(isLoROM);
      } break;

    case 'v':
      { //Return fimware version
        Serial.write(FIRMWARE_ID);
        Serial.write((char)FIRMWARE_VERSION);

      } break;

    case 'z':
    case 'Z':
      {
        st018_biosDump(cmd);
      } break;
  }
}
