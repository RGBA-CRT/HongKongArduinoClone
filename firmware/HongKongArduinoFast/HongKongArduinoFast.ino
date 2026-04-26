/* こーどねーむ「ホンコン」 with Arduino 高速版ファームウェア
Original: Taniyama 2016/2/14 [https://susumutaniyama.github.io/HongKongArduino/]
Modification: RGBA_CRT 2016/3/19 [rgba3crt1p@gmail.com]
Some code is referred on [https://github.com/sanni/cartreader/].
Protocol notes: https://github.com/RGBA-CRT/HongKongArduinoClone/wiki/Firmware-docs
*/

/* version history
 * HKAF0: 2017/02: add version cmd
 * HKAF1: 2018/02: dynamic baud rate, speedup: 82KB/s
 * HKAF2: 2018/03: protocol change
 * HLAF3: 2018/07: clock cmd change
 * HKAF4: 2029/03: ST017 surpport
 * HKAF5: 2025/08: flash write
 */

#pragma GCC push_options
#pragma GCC optimize("O3")
#include "bus_io.h"
#include <assert.h>

#define INITIAL_BAUDRATE 115200

#define HKAC_DEBUG
#ifndef HKAC_DEBUG
#define FIRMWARE_NAME "HKAF"
#define FIRMWARE_VERSION "5"  // FWのAPIが変わったらインクリメント
#else
#define FIRMWARE_NAME "HKAD"  // debug branch
#define FIRMWARE_VERSION "3"
#endif

const char* FIRMWARE_ID = (FIRMWARE_NAME FIRMWARE_VERSION);

#define LARGE_RX_BUFFER_LEN 0x400  //ホスト側とサイズを合わせる
static_assert((LARGE_RX_BUFFER_LEN % 512) == 0, "RX_BUFFER is must be multiple value of page_size");
uint8_t large_rx_buf[LARGE_RX_BUFFER_LEN];

//CONFIG
#define _ENABLE_CIC
#define ENABLE_ST018_BIOS_DUMP
#define ENABLLE_SFMEM
// #define BUILD_SUPER_SLOW_READ

//--------------------------------------------

uint8_t gflags;
#define FLASH_CONFIG_CEOE_SWAP 0x01
#define FLASH_CONFIG_BYTE_VERIFY 0x02
#define GFLAGS_SUPER_SLOW_READ 0x04
#define FLASH_CONFIG_VERIFY_FIXED_VALUE 0x08

// NOP generator
#if (F_CPU <= 16000000ULL)
#define DELAY_FRACT 1
#elif (F_CPU <= 32000000ULL)
#define DELAY_FRACT 2
#else
#error "please config DELAY_FRACT"
#endif
#define nop_generate(x) __asm__ volatile(".rept " #x " \n\t nop \n\t .endr")
#define minimum_delay(x) nop_generate(x)
#define wait_62500_psec(x) minimum_delay((x * DELAY_FRACT))

bool haveClockModule(void);


uint8_t dbg = 0;
void readCart(uint8_t isLoROM) {
  while (serial_available() < 5)
    ;
  uint16_t address = serial_read_word();
  uint8_t bank = serial_read();
  uint16_t datasize = serial_read_word();
  BB_DIR_INPUT();
  dbg = 0;

  // note: 一見最適化の余地があるが、結局serial_sendで待ちが発生するので意味無し
  do {
    setAddress(bank, address++, isLoROM);
    // SA1でおかしくなったらsetAddressにnopを仕込む

    serial_send(readData());
  } while (--datasize);
}

void writeCart(int isLoROM = false) {
  //コマンド受信
  while (serial_available() < 5)
    ;
  uint16_t address = serial_read_word();
  uint8_t bank = serial_read();
  uint16_t datasize = serial_read_word();

  CART_WRITE_DISABLE();
  CART_OUTPUT_DISABLE();
  BB_DIR_OUTPUT();

  uint16_t goalAdr = address + datasize;
  uint16_t bufpos = LARGE_RX_BUFFER_LEN;  //buffer ptr, 最初は必ず受信させる

  while (1) {
    //データ受信
    if (bufpos >= LARGE_RX_BUFFER_LEN) {
      hostsync_receive(LARGE_RX_BUFFER_LEN);
      bufpos = 0;
    }

    write_byte_cart(bank, address, large_rx_buf[bufpos]);
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

void lazy_wait_1us() {
  wait_62500_psec(16);
}

// とてもざっくり不正確なウェイト
void lazy_wait_us(word lazy_wait_us) {
  for (word i = 0; i < lazy_wait_us; i++) { lazy_wait_1us(); }
}


void setup() {
  // Serial.begin(INITIAL_BAUDRATE, SERIAL_CONFIG);
  // 115200
  serial_begin(INITIAL_BAUDRATE);

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

#ifdef _ENABLE_CIC
  //クロックジェネレータの動作を開始
  // setupCloclGen_(true, false, true, false);

  setupCloclGen(true, false, true, false);
#endif

  digitalWrite(WE, HIGH);
  digitalWrite(RST, LOW);

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

  // Pull-Donw
  // MCUCR |= 0x10;
}

void loop() {
  while (serial_available() == 0)
    ;  //wait command
  uint8_t cmd = serial_read();

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
        while (serial_available() < 3)
          ;
        uint8_t isLoROM = (cmd == 'a');
        uint16_t address = serial_read_word();
        uint8_t bank = serial_read();
        setAddress(bank, address, isLoROM);
      }
      break;

    case 'b':
      {  //Set boudrate
        while (serial_available() < 4)
          ;

        unsigned long new_baudrate = serial_read()
                                     | (unsigned long)serial_read() << 8
                                     | (unsigned long)serial_read() << 16
                                     | (unsigned long)serial_read() << 24;

        serial_begin(new_baudrate);
        //Serial.flush();
        //このあと、ファームチェックで値が正常に帰ってくることを確認してから
        //各種コマンドを投げてください
        setAddress(0xFF, 0xFFFF, false);
      }
      break;

    case 'c':
      {  //set control bus(OE WD RST CS)
        while (serial_available() < 1)
          ;
        setCtrlBus(serial_read());
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
        while (serial_available() < 1)
          ;
        uint8_t mode = serial_read();
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
        serial_send('0' | haveClockModule());
      }
      break;

    case 'i':
      {  // print infomation
        serial_send_text(FIRMWARE_ID);
        serial_send(FIRMWARE_VERSION);
#ifdef _ENABLE_CIC
        serial_send_text("-CIC ");
        serial_send_text(haveClockModule() ? "[con]" : "[dis]");
#endif
        // Serial.print("\nFash:\n");
        // Serial.print(flash_bank, HEX);        Serial.print(flash_address[0], HEX);        Serial.print(":");        Serial.print(flash_cmd[0], HEX);        Serial.print(",\t");
        // Serial.print(flash_bank, HEX);        Serial.print(flash_address[1], HEX);        Serial.print(":");        Serial.print(flash_cmd[1], HEX);        Serial.print(",\t");
        // Serial.print(flash_bank, HEX);        Serial.print(flash_address[2], HEX);        Serial.print("\n");
        // for (uint8_t i = 0; i < 20; i++) {
        //   Serial.print((char)read_byte_cart(0x00, 0xffc0 + i));
        // }  Serial.print("\n");

        serial_send_text("DBG_");
        extern long tx_spin_count;
        serial_send(dbg);
        serial_send(tx_spin_count >> 24);
        serial_send(tx_spin_count >> 16);
        serial_send(tx_spin_count >> 8);
        serial_send(tx_spin_count);
        serial_send('0' + DELAY_FRACT);
      }
      break;

    case 's':
      {  // set register(1uint8_t write)
        serial_send_text((char)read_byte_cart(0xc0, 0x0000));
        write_byte_cart(0x00, 0x2220, 04);
        serial_send_text((char)read_byte_cart(0xc0, 0x0000));
      }
      break;

#ifdef ENABLLE_SFMEM
    case 'S':
      {  // setup SF memory
        digitalWrite(OE, HIGH);
        digitalWrite(CS, LOW);
        digitalWrite(WE, HIGH);
        digitalWrite(RST, HIGH);

        write_byte_cart(0x00, 0x2400, 0x09);
        uint8_t status = read_byte_cart(0x00, 0x2400);
        write_byte_cart(0x00, 0x2401, 0x28);
        write_byte_cart(0x00, 0x2401, 0x84);
        write_byte_cart(0x00, 0x2400, 0x06);
        write_byte_cart(0x00, 0x2400, 0x39);

        if (read_byte_cart(0x00, 0x2400) == 0x2A)
          serial_send_text("OK");
        else
          serial_send_text("NG");
        serial_send(read_byte_cart(0x00, 0x2400));
      }
      break;
#endif

    case 'T':
    case 't':
      {  // set register
        while (serial_available() < 4)
          ;
        uint8_t bank = serial_read();
        uint16_t address = serial_read_word();
        uint8_t data = serial_read();

        // 't'の場合はLoROMアドレス換算していたが廃止

        write_byte_cart(bank, address, data);
      }
      break;

    case 'W':
    case 'w':
      {  //bulk write cart
        uint8_t isLoROM = (cmd == 'w');
        writeCart(isLoROM);
      }
      break;

    case 'v':
      {  //Return fimware version
        serial_send_text(FIRMWARE_ID);
        setAddress(0x00, 0, false);
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
      serial_send_text("?CMD=");
      serial_send(cmd);
      serial_send_text("\xAA\xAA");
  }
}

#pragma GCC pop_options
