#pragma GCC push_options
#pragma GCC optimize("Ofast")

void inline SetFlashOECtrl(bool swap_ce_oe) {
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
//--------------
//   ic level
//--------------
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

//--------------
// snes level
//--------------
#define LO_TO_REAL_ADDRESS(bank, address) \
  { \
    bank = (bank << 1) | (address >> 15); \
    address |= 0x8000; \
  }


inline void setAddress(uint8_t bank, uint16_t address, uint8_t isLoROM) {
  if (isLoROM) {
    LO_TO_REAL_ADDRESS(bank, address);
  }

  BB_OUT_DISABLE();
  setAddressFFs(bank, address);
  // lazy_wait_1us();
}

uint8_t readData() {
// #define UART_DEBUG
#ifdef UART_DEBUG
  return dbg++;
#endif

  CART_OUTPUT_ENABLE();
  dataPinDirInput();
  BB_OUT_ENABLE();

  // OLD: 1000ns(16)
  // TESTED AT 2026/03 & SA1 ROM DUMP, 125ns(2)
  // TESTED AT 2026/03 & SA1バス釣り ROM DUMP, 125ns(2)
  // AddressOutputDelay: 110nsぐらい
  // Output Enable to Output Delay: 25nsぐらい
  // Arduino Uno@16MHzでToggle nop2回 Toggle で200nsぐらい。
  // 立ち上がり直前でラッチしたいので、前準備に時間かけていいけどRead後は小さくすると良い
  wait_62500_psec(2);
#ifdef BUILD_SUPER_SLOW_READ
  uint8_t b;
  if (gflags & GFLAGS_SUPER_SLOW_READ) {
    const uint8_t ok_retry_max = 10;
    const uint8_t ng_retry_max = 30;
    uint8_t ok_cnt = ok_retry_max;
    uint8_t ng_cnt = ng_retry_max;
    uint8_t err = 0, b2;
retry:
    b = getDataPin();
    lazy_wait_1us();
    b2 = getDataPin();
    if (b2 == b) {
      ng_cnt = ng_retry_max;
      if (--ok_cnt) { goto retry; }
    } else {
      ok_cnt = ok_retry_max;
      err++;
      if (--ng_cnt) { goto retry; }
    }
    b ^= b2;  // error bit report
    // b = err; // error count report
    // b = b2;
  } else {
    b = getDataPin();
  }
#else
  uint8_t b = getDataPin();
#endif

  CART_OUTPUT_DISABLE();

  BB_OUT_DISABLE();
  dataPinDirOutput();
  return b;
}

// /REを制御して読み込む
uint8_t read_byte_cart(uint8_t bank, uint16_t address) {
  setAddress(bank, address, false);

  // pre /OE pulse
  wait_62500_psec(4);

  CART_OUTPUT_ENABLE();

  // NOPの数検証済み 4LINE（SA1のSRAM WRITE VERIFY）
  wait_62500_psec(16);

  uint8_t ret = readData();

  CART_OUTPUT_DISABLE();
  return ret;
}

//　/WRとかをちゃんと制御して書き込む
void write_byte_cart(uint8_t bank, uint16_t address, uint8_t data) {
  setAddress(bank, address, false);
  setDataPin(data);

  BB_DIR_OUTPUT();
  BB_OUT_ENABLE();

  // /WEパルス成立 & 74HC245 -> SFC へのデータ安定化のWAIT
  // 74HC245 -> SFC へのデータ安定化
  // OLD (8)
  // TESTED AT 2026/03 & SA1バス釣り SRAM WRITE, 62.5ns(1)
  wait_62500_psec(1);

  CART_WRITE_ENABLE();

  // /WEパルスの時間稼ぎ
  // OLD: (16)
  // TESTED AT 2026/03 & SA1 SRAM WRITE, 380ns(6)
  // TESTED AT 2026/03 & SA1バス釣り SRAM WRITE, 312.5ns
  wait_62500_psec(6);

  CART_WRITE_DISABLE();

  BB_OUT_DISABLE();
  BB_DIR_INPUT();
}

#pragma GCC pop_options