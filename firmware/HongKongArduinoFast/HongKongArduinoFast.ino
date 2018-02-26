/* こーどねーむ「ホンコン」 with Arduino 高速版ファームウェア
   Original : たにやま 2016/2/14 [http://hongkongarduino.web.fc2.com/]
   Optimize : RGBA_CRT 2016/3/19 [rgba3crt1p@gmail.com]
   Some codes are referenced to [https://github.com/sanni/cartreader/]
   History:
    2016/7/2  Add LastAdr(Max55KB/s)
    2016/8/29 SnesCIC対応
    2017/2/15 [HKAF0]ファームチェック導入
    2018/2/26 [HKAF1]動的ボーレート
    2018/2/26 bankを超える吸出しは不可に(62Kb/s)
*/

//config
//シリアルコンバータがCH340の場合1000000bpsが限界
#define INITIAL_BAUDRATE 115200
#define SERIAL_CONFIG SERIAL_8N1
#define FIRMWARE_ID "HKAF"
#define FIRMWARE_VERSION '1'
#define BUFFER_LEN 128

//クロック回路有効/無効
#define _ENABLE_CIC

/*
  Reference :
    'r' or 'R' の引数はアドレス(3バイト)とデーターサイズ(3バイト)で、
               指定したアドレスからデーターサイズ分を送信

    'w' or 'W' の引数はアドレス(3バイト)とデーターサイズ(3バイト)で、
               アドレスをインクリメントしながら書き込んでいく

    'a' or 'A' の引数はアドレス(３バイト)　アドレスバスを操作

    'c'　引数はフラグ（１バイト）で、/CE /OE /WE RESET等を操作
    'v'  引数なし。バージョンを返します。FIRMWARE_ID+FIRMWARE_VERSIONの形式
    'g'  クロック関連　引数は'0'か'1'
    'b'　ボーレートを変更 引数は32ビットリトルエンディアン
*/
/*
  接続方法：
  　1.INITIAL_BAUDRATE[bps]で通信開始
    2.'v'コマンドを送る, 正常にFIRMWARE_IDが返ってくるまでリトライ
    3.'b'コマンドでボーレートを1000000bpsとかにする。
    4.手順2と同じく'v'コマンドで応答が来るまで待つ。
      この時に応答がなければそのボーレートは使用不可
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
inline void setAddress(byte bank, word address, int isLoROM)
{
  if (isLoROM) {
    //address = (address & 0xFF8000) << 1 | (address & 0x007FFF) | 0x8000 ;
    bank = (bank << 1) | (address >> 15);
    address |= 0x8000;
    //address = address / 0x8000 * 2 * 0x8000 + address % 0x8000 + 0x8000;
  }

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

  if (lastadr[2] != bank) {
    outCh(2, bank);
    lastadr[2] = bank;
  }
}

inline unsigned char readData()
{
  setDataDir(INPUT);

  //PWM対応ポートの場合ただPORTBをいじってもダメらしい
  Enable74HC245();

  byte b = (PIND >> 2) | ((PINB << 6));

  // digitalWrite(GD, HIGH);
  Disable74HC245();
  setDataDir(OUTPUT);

  return b;
}


#define CART_WRITE_ENABLE()   PORTC &= 0b11101111
#define CART_WRITE_DISABLE()  PORTC |= 0b00010000

//ttps://github.com/sanni/cartreader/blob/master/Cart_Reader/SNES.ino
//要修正
void writeSRAM(int isLoROM = false) {
  while (Serial.available() < 7)
    delay(1);

  word address = Serial.read();
  address += ((unsigned long)Serial.read() << 8);
  byte bank = Serial.read();

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

  CART_WRITE_DISABLE();
  setDataDir(OUTPUT); //アドレスセットもデーターセットも出力
  digitalWrite(DIR, HIGH); // Arduino => [A_Bus_IN]-[B_Bus_OUT] => SFC

  byte data;
  unsigned long goalAdr = address + datasize;
  while (address < goalAdr) {
    while (Serial.available() < 1);
    data = Serial.read();

    //アドレス出力によりデータバス出力停止
    CART_WRITE_DISABLE();
    Disable74HC245();
    setAddress(bank, address, isLoROM);

    //ENABLEの時間をなるべく長くしておく
    CART_WRITE_ENABLE();
    setData(data);

    Enable74HC245();
    address++;
  }

  CART_WRITE_DISABLE();
  Disable74HC245();
  digitalWrite(DIR, LOW); //Arduino <= [A_Bus_OUT]-[B_Bus_IN] <= SFC

}

inline void setCtrlBus(byte b) {
  digitalWrite(OE , (b & 0b0001) ? HIGH : LOW);
  digitalWrite(CS , (b & 0b0010) ? HIGH : LOW);
  digitalWrite(WE , (b & 0b0100) ? HIGH : LOW);
  digitalWrite(RST, (b & 0b1000) ? HIGH : LOW);
}

#ifdef _ENABLE_CIC
//[Nintendo Cart Reader]より
void setupCloclGen() {
  // Adafruit Clock Generator
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

void setup()
{
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
  //setAddressは差分しかセットしないので、手動でFlipFlopを初期化
  setDataDir(OUTPUT);
  outCh(0, 0x00); lastadr[0] = 0;
  outCh(1, 0x00); lastadr[1] = 0;
  outCh(2, 0x00); lastadr[2] = 0;

  Serial.begin(INITIAL_BAUDRATE, SERIAL_CONFIG);
}

#define Serial_readWord() ((word)Serial.read() | ((word)Serial.read() << 8))

inline void readRom(byte isLoROM) {
  byte buf[BUFFER_LEN];
  while (Serial.available() < 6);
  word address = Serial_readWord();
  byte bank = Serial.read();

  word datasize = Serial_readWord();
  Serial.read();  //互換性のためだけ

  word goalAdr = address + datasize;
  word i = 0;
  while (1) {
    setAddress(bank, address, isLoROM);
 /*   buf[i] = readData();
    address++; i++;
    if (i >= BUFFER_LEN) {
      Serial.write(buf, i);
      i = 0;
    }*/
    Serial.write(readData());
    address++;
    if (address == goalAdr) break; //00:C000-01:0000
  }
  if (i != 0)Serial.write(buf, i);
}
void loop() {
  while (Serial.available() == 0);  //wait command
  byte cmd = Serial.read();

  switch (cmd) {
    case 'R':
    case 'r':
      readRom((cmd == 'r'));
      break;

    case 'c':
      { //set control bus(OE WD RST CS)
        while (Serial.available() < 1) ;
        setCtrlBus(Serial.read());
      } break;

    case 'a':
    case 'A':
      { //Set address
        while (Serial.available() < 3)  ;
        byte isLoROM = (cmd == 'a');
        word address = Serial_readWord();
        byte bank = Serial.read();
        setAddress(bank, address, isLoROM);
      } break;

    case 'W':
    case 'w':
      { //Write cart
        byte isLoROM = (cmd == 'w');
        writeSRAM(isLoROM);
      } break;

    case 'v':
      { //Return fimware version
        Serial.write(FIRMWARE_ID);
        Serial.write((char)FIRMWARE_VERSION);

      } break;

    case 'g':
      { //CPU ClockGen Start/Stop
        while (Serial.available() < 1);
        byte mode = Serial.read();
#ifdef _ENABLE_CIC
        if (mode == '1') {
          clockgen.set_freq(357954500ULL, SI5351_PLL_FIXED, SI5351_CLK1);
          clockgen.output_enable(SI5351_CLK1, 1);
        } else {
          clockgen.output_enable(SI5351_CLK1, 0);
        }
#endif
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

        break;
      }
  }
}


