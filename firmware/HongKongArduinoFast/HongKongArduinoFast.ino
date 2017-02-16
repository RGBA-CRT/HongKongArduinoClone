//こーどねーむ「ホンコン」 with Arduino 高速版ファームウェア
//Original : たにやま 2016/2/14 [http://hongkongarduino.web.fc2.com/]
//Optimize : RGBA_CRT 2016/3/19 [rgba3crt1p@gmail.com]
//2016/7/2  Add LastAdr
//2016/8/29 SnesCIC対応
//2017/2/15 マジックナンバー追加

//[HongKongArduinoFast.hex]は-O2でコンパイルされたバイナリです。
//通常は↑を書き込んで使ってください
//ボーレートはお使いのArduinoのデータシートを参考に設定してください↓
#define SERIAL_BAUDRATE 500000*2 //62.5KB/s * 2
#define FIRMWARE_ID "HKAF"
#define FIRMWARE_VERSION '0'

/*
  Reference :
    'r' or 'R' の引数はアドレス(3バイト)とデーターサイズ(3バイト)で、
               指定したアドレスからデーターサイズ分を送信します。

    'w' or 'W' の引数はアドレス(3バイト)とデーターサイズ(3バイト)で、
               アドレスをインクリメントしながら書き込んでいきます。

    'a' or 'A' の引数はアドレス(３バイト)で、アドレスバスを操作できます。

    'c'　の引数はフラグ（１バイト）で、/CE /OE /WE RESET等を操作します。
    'v'  引数なし。バージョンを返します。FIRMWARE_ID+FIRMWARE_VERSIONの形式です。

  コードは以下の方針でいきます
  ・ループを展開
  ・見易さよりも速度優先
  ・レガシーなCの文法にこだわらない
  ・コンパイルオプションO1 O2 O3で一番早かったものを使用
*/
/* ENABLLE_CICモードの場合
  　・Si5351なし起動 ：
    何かつながっていると(LEDとか)、ACK待ちでフリーズ
    開放状態だとOK
  　・Si5351あり、SuperCICなし -> SuperFXの場合不安定？
  　・Si5351あり、SuperCICあり -> OK
  　・カートリッジをつないだまま初期化すると、ACK待ちでフリーズ
*/

//----------------- 実験コード ------------------
#define _ENABLE_CIC
#ifdef _ENABLE_CIC

#include "si5351.h"
#include "Wire.h"
#include <avr/io.h>

//I2C通信状態を解除してA4,A5ピンを使用可能に
#define DISABLE_I2C() (TWCR = 0)

//クロックジェネレータ
Si5351 clockgen;
#endif
//--------------------------------------------

//#include<wiring.c>

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

//コントロールピン
#define DIR  14
#define CK  15
#define OE  16
#define CS  17
#define WE  18
#define RST  19

#define Disable74HC245() PORTB |= 0b00000100
#define Enable74HC245()   {volatile uint8_t oldSREG = SREG;cli();PORTB &= 0b11111011;SREG = oldSREG;}
//#define Enable74HC245()  digitalWrite(GD, LOW)

//現在のアドレスの状態
byte lastadr[3];

//データピンの方向設定
inline void setDataDir(int DATADIR)
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

//アドレスバスを構成する74HC377へ値をセット
inline void outCh(int ch, byte b)
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

//アドレスバスを設定
inline void setAddress(unsigned long address, int isLoROM)
{
  if (isLoROM)
    address = address / 0x8000 * 2 * 0x8000 + address % 0x8000 + 0x8000;

  //変更のないFlipFlopはいじらない
  byte spritAdr = address;
  if (lastadr[0] != spritAdr) {
    outCh(0, spritAdr);
    lastadr[0] = spritAdr;
  }

  spritAdr = address >> 8;
  if (lastadr[1] != spritAdr) {
    outCh(1, spritAdr);
    lastadr[1] = spritAdr;
  }

  spritAdr = address >> 16;
  if (lastadr[2] != spritAdr) {
    outCh(2, spritAdr);
    lastadr[2] = spritAdr;
  }
}

inline unsigned char readData()
{
  setDataDir(INPUT);

  //PWM対応ポートの場合ただPORTBをいじってもダメらしい
  Enable74HC245();
  //digitalWrite(GD, LOW); // Ch Enable
  //turnOffPWM2(digitalPinToTimer(GD));
  //PORTB &= 0b11111011;

  byte b = (PIND >> 2) | ((PINB << 6));
  /*if (PIND & 0b00000100)       b |= (1 << 0); //if (digitalRead(DATA0 + 0) == HIGH)      b |= (1 << 0);
    if (PIND & 0b00001000)       b |= (1 << 1);
    if (PIND & 0b00010000)       b |= (1 << 2);
    if (PIND & 0b00100000)       b |= (1 << 3);
    if (PIND & 0b01000000)       b |= (1 << 4);
    if (PIND & 0b10000000)       b |= (1 << 5);
    if (PINB & 0b00000001)       b |= (1 << 6); // if (digitalRead(DATA0 + 6) == HIGH)       b |= (1 << 6);
    if (PINB & 0b00000010)       b |= (1 << 7); // if (digitalRead(DATA0 + 7) == HIGH)       b |= (1 << 7);
  */

  // digitalWrite(GD, HIGH);
  Disable74HC245();
  setDataDir(OUTPUT);

  return b;
}

void writeSRAM(int isLoROM = false) {
  while (Serial.available() < 7)
    delay(1);

  unsigned long address = Serial.read();;
  address += ((unsigned long)Serial.read() << 8);
  address += ((unsigned long)Serial.read() << 8 * 2);

  unsigned long datasize = Serial.read();
  datasize += ((unsigned long)Serial.read() << 8);
  datasize += ((unsigned long)Serial.read() << 8 * 2);

  /*
    1.74HC245を無効にしてアドレスバスをセットする(OUTPUT)
    2.それぞれのチャンネルの有効/無効はoutChがやってくれる->74HC377無効
    3.74HC245を有効にしてデータ方向をSFC側へ書き込むようにする
    4.書き込む値を受信してデータバスにセット
    5.goto 1
  */

  setDataDir(OUTPUT); //アドレスセットもデーターセットも出力
  digitalWrite(DIR, HIGH); // Arduino => [A_Bus_IN]-[B_Bus_OUT] => SFC

  unsigned long goalAdr = address + datasize;
  while (address < goalAdr) {
    if (Serial.available() != 0) {
      Disable74HC245();
      setAddress(address, isLoROM);

      Enable74HC245();
      setData((byte)Serial.read());

      address++;
    } else {
      delay(1);
    }
  }

  Disable74HC245();
  digitalWrite(DIR, LOW); //Arduino <= [A_Bus_OUT]-[B_Bus_IN] <= SFC

}

inline void setCtrlBus(char b) {
  digitalWrite(OE , (b & B0001) ? HIGH : LOW);
  digitalWrite(CS , (b & B0010) ? HIGH : LOW);
  digitalWrite(WE , (b & B0100) ? HIGH : LOW);
  digitalWrite(RST, (b & B1000) ? HIGH : LOW);
}

#ifdef _ENABLE_CIC
//[Nintendo Cart Reader]より
void setupCloclGen() {
  // Adafruit Clock Generator
  //clockgen.set_correction(-29000);
  clockgen.set_correction(0);
  clockgen.init(SI5351_CRYSTAL_LOAD_8PF, 0);
  clockgen.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  clockgen.set_pll(SI5351_PLL_FIXED, SI5351_PLLB);
  clockgen.set_freq(2147727200ULL, SI5351_PLL_FIXED, SI5351_CLK0);
  clockgen.set_freq(307200000ULL, SI5351_PLL_FIXED, SI5351_CLK2);
  clockgen.output_enable(SI5351_CLK0, 1);
  clockgen.output_enable(SI5351_CLK1, 0);
  clockgen.output_enable(SI5351_CLK2, 1);
}
#endif

#define CLK 2
#define PS 3
#define DAT 4

void setup()
{
  // Serial.begin(115200);
#ifdef __DEBUG_LCD
  lcd.begin(8, 2);
  lcd.setContrast(8);
  print("HKAF0 bps=", 0, 1);
  print(SERIAL_BAUDRATE, 0, 0);
#endif

#ifdef _ENABLE_CIC
  //クロックジェネレータの動作を開始
  setupCloclGen();
  delay(100);
  DISABLE_I2C();
#endif

  //コントロールピンをすべてOUTPUTに
  for (int i = GD; i <= RST; i++) {
    pinMode(i, OUTPUT);
  }

  digitalWrite(GD, LOW);  //GD(PORTB 02) PWM DISABLE
  digitalWrite(GD, HIGH); // Disable
  digitalWrite(G0, HIGH); // Disable
  digitalWrite(G1, HIGH); // Disable
  digitalWrite(G2, HIGH); // Disable

  digitalWrite(CK, LOW);

  digitalWrite(DIR, LOW); // 入力

  digitalWrite(OE, LOW);
  digitalWrite(CS, LOW);
  digitalWrite(WE, HIGH);
  digitalWrite(RST, HIGH);

  //アクセスランプを消灯＆アドレスバス初期化
  //setAddressは差分しかセットしないので、手動でFFを初期化
  setDataDir(OUTPUT);
  outCh(0, 0x00); lastadr[0]=0;
  outCh(1, 0x00); lastadr[1]=0;
  outCh(2, 0x00); lastadr[2]=0;

  Serial.begin(SERIAL_BAUDRATE);
}

void loop() {
  while (Serial.available() == 0) {

  }
  byte cmd = Serial.read();


  switch (cmd) {
    case 'R':
    case 'r': {
        while (Serial.available() < 6);

        byte isLoROM = (cmd == 'r');
        unsigned long address = Serial.read();
        address |= ((unsigned long)Serial.read() << 8);
        address |= ((unsigned long)Serial.read() << 16);

        unsigned long datasize = Serial.read();
        datasize |= ((unsigned long)Serial.read() << 8);
        datasize |= ((unsigned long)Serial.read() << 16);

        unsigned long int goalAdr = address + datasize;
        while (address < goalAdr) {
          setAddress(address, isLoROM);
          Serial.write(readData());
          address++;
        }
        break;
      }

    case 'c':
      {
        while (Serial.available() < 1) ;
        byte b = Serial.read();
        setCtrlBus(b);
        break;
      }

    case 'a':
    case 'A': {
        while (Serial.available() < 3)  ;
        byte isLoROM = false;
        if (cmd == 'a') isLoROM = true;
        unsigned long address = Serial.read();
        address += ((unsigned long)Serial.read() << 8);
        address += ((unsigned long)Serial.read() << 8 * 2);
        setAddress(address, isLoROM);
        break;
      }

    case 'W':
    case 'w': {
        byte isLoROM = false;
        if (cmd == 'w') {
          isLoROM = true;
        }
        writeSRAM(isLoROM);
        break;
      }

    case 'v': {
        Serial.write(FIRMWARE_ID);
        Serial.write((char)FIRMWARE_VERSION);
        break;
      }
  }

  // while (Serial.available() != 0) { Serial.read(); print("dust",0,0);}

}


