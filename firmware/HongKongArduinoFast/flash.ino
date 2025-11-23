#pragma GCC push_options
#pragma GCC optimize("O3")


#define FLASH_OE_ENABLE() PORTC &= val_oe_and_mask
#define FLASH_OE_DISABLE() PORTC |= val_oe_or_mask
#define FLASH_OE_TOGGLE() PINC = val_oe_or_mask

#define FLASH_CE_ENABLE() PORTC &= ~val_ce_or_mask
#define FLASH_CE_DISABLE() PORTC |= val_ce_or_mask
#define FLASH_CE_TOGGLE() PINC = val_ce_or_mask

//flash config
#define FLASH_COMMAND_LENGTH 3
// static byte flash_bank;
static byte flash_banks[FLASH_COMMAND_LENGTH];
static word flash_address[FLASH_COMMAND_LENGTH];  // = {0xAAAA,0x5555,0xAAAA}
static byte flash_cmd[FLASH_COMMAND_LENGTH];      // = {0xAA  ,0x55,  [cmd] }
static byte flash_byte_verify;
static byte flash_page_size;
static byte flash_page_wait;
static byte flash_verify_fixed_value;
static byte flash_verify_byte;
#define FLASH_PAGE_WRITE_EXPECTED_STATUS 0x80

void flashReceiveConfig() {
  // flash_bank + (flash_address,flash_cmd) * FLASH_COMMAND_LENGTH
  while (Serial.available() < (FLASH_COMMAND_LENGTH * 4))
    ;
  // flash_bank = Serial.read();
  for (byte i = 0; i < FLASH_COMMAND_LENGTH; i++) {
    flash_banks[i] = Serial.read();
    flash_address[i] = Serial_readWord();
    flash_cmd[i] = Serial.read();
  }

  while (Serial.available() < 4)
    ;
  gflags = Serial.read();
  SetFlashOECtrl((gflags & FLASH_CONFIG_CEOE_SWAP));
  flash_byte_verify = gflags & FLASH_CONFIG_BYTE_VERIFY;
  flash_verify_fixed_value = gflags & FLASH_CONFIG_VERIFY_FIXED_VALUE;

  flash_page_size = Serial.read();
  flash_page_wait = Serial.read();
  flash_verify_byte = Serial.read();

#if 0
  Serial.print("FMT25112301\n");
  for (byte i = 0; i < FLASH_COMMAND_LENGTH; i++) {
    Serial.print(flash_banks[i],HEX);
    Serial.print(flash_address[i],HEX);
    Serial.print(" <- ");
    Serial.println(flash_cmd[i],HEX);
  }
  Serial.println(gflags,HEX);
  Serial.println(flash_byte_verify ? "ByteVeri" : "PageVeri");
  Serial.println(flash_verify_fixed_value ? "FixdVeri" : "AutoVeri");
  Serial.println(flash_page_size,HEX);
  Serial.println(flash_page_wait,HEX);
  Serial.println(flash_verify_byte,HEX);
#endif
}

inline void bulkReadInit() {
  setDataPin(0x00);
  dataPinDirInput();
  BB_DIR_INPUT();
}
inline void bulkReadExit() {
  BB_DIR_TOGGLE();
  dataPinDirOutput();
}

inline byte bulkReadAcquire() {
  FLASH_OE_TOGGLE();

  __asm__ volatile("nop");

  byte b = getDataPin();

  FLASH_OE_TOGGLE();

  return b;
}


// 500usぐらいをtimeoutとしたい
// clockは16MHz, 1cycle1命令とする(今もそうなのか？). 62.5ns
// readDataが108clkぐらい
// その他関数内のループを少なく見積もって5clkぐらい。7,062.5ns
#define FLASH_WAIT_TIMEOUT_CYCLE 0xffff
// #define POLL_ONLY_DQ7

word max_loop = FLASH_WAIT_TIMEOUT_CYCLE;
inline byte flashWaitOperation(byte expect_byte) {
  bulkReadInit();
  byte ret;
  byte ok_cnt = 2;
  word i = FLASH_WAIT_TIMEOUT_CYCLE;
  // S29L032N tBusy
  // __asm__ volatile("nop");
  // __asm__ volatile("nop");
  // __asm__ volatile("nop");

  do {
    ret = bulkReadAcquire();
    if (ret == expect_byte) {
      if (--ok_cnt) continue;
      break;
    }
  } while (--i);
  bulkReadExit();

  max_loop = (max_loop > i) ? i : max_loop;
  return ret;
}

void flashBulkWriteInit() {
  // 初期PIN状態。ループ内最適化のためtoggleなどしているため注意。
  CART_WRITE_DISABLE();
  FLASH_OE_DISABLE();
  BB_DIR_OUTPUT();
  BB_OUT_ENABLE();
  FLASH_CE_ENABLE();
}

void flashBulkWriteExit() {
  FLASH_CE_DISABLE();
  CART_WRITE_DISABLE();
  FLASH_OE_DISABLE();
  BB_OUT_DISABLE();
  BB_DIR_INPUT();
}

//　/WRとかをちゃんと制御して書き込む
void writebyte_cart2(byte bank, word address, byte data) {
  setAddress_(bank, address);
  setDataPin(data);

  // tAS: Address Setup Time: アドレスを出力してからWEを下げていいまで: 0ns@S29GL032, 0ns@MX29F1610, 0ns@LV640
  __asm__ volatile("nop");  // 100ns
  __asm__ volatile("nop");  // 100ns

  CART_WRITE_TOGGLE();

  // tAH: Address Hold Time: WEが下がってからアドレスを潰していいまで: 45ns@S29GL032, 40ns@MX29F1610, 45ns@LV640
  // tDS: data setup time: データが出てからWEが立ち上がるまで: 35ns@S29GL032, 50ns@MX29F1610, 45ns@LV640
  // tDH: Data Hold Time: WE立ち上がってから潰して良いまで: 0ns@S29GL032, 0ns@LV640
  // tWP: WE-Lowパルスの幅: 35ns@S29GL032, 55ns@MX29F1610, 30ns@LV640
  __asm__ volatile("nop");  // 100ns
  __asm__ volatile("nop");  // 100ns

  CART_WRITE_TOGGLE();
  // tWPH: WE-Highパルスの幅: 50ns @ MX29F1610
  __asm__ volatile("nop");  // 100ns
}

void sendErrorReport(byte bank, word address, byte real_byte, byte expected_byte) {
  serial_send('X');
  serial_send((byte)address);
  serial_send((byte)(address >> 8));
  serial_send(bank);
  serial_send(expected_byte);
  serial_send(real_byte);
  serial_send('X');
}

#define GetProgDoneResponce(x) (flash_verify_fixed_value ? flash_verify_byte : x)

void flashByteProgram(byte bank, word address, word datasize) {
  // メモリは余っているので速度優先でじゃんじゃんつかおう
  byte* bufptr = buf;
  word remain = 1;  // 最初は必ず受信させる
  byte page_count = flash_page_size;

  do {
    //データ受信
    if (!(--remain)) {
      hostsync_receive(RX_BUFFER_LEN);
      bufptr = buf;
      remain = RX_BUFFER_LEN;
    }

    writebyte_cart2(flash_banks[0], flash_address[0], flash_cmd[0]);
    writebyte_cart2(flash_banks[1], flash_address[1], flash_cmd[1]);
    writebyte_cart2(flash_banks[2], flash_address[2], flash_cmd[2]);

    // output program byte
    byte b = *bufptr;
    writebyte_cart2(bank, address, b);

    address++;
    bufptr++;

    byte expect = GetProgDoneResponce(b);
    byte wait_ret = flashWaitOperation(expect);
    if (wait_ret != expect) {
      sendErrorReport(bank, --address, wait_ret, expect);
      break;
    }
  } while (--datasize);
}

void flashPageProgram(byte bank, word address, word datasize) {
  // メモリは余っているので速度優先でじゃんじゃんつかおう
  byte* bufptr = buf;
  word remain = flash_page_size;  // 最初は必ず受信させる
  byte page_count = 1;            // 最初は必ずページ跨ぎ処理（コマンド発行）をする
  bool initial = true;

  do {
    if (!(--page_count)) {
      //データ受信
      remain -= flash_page_size;
      if (!remain) {
        hostsync_receive(RX_BUFFER_LEN);
        bufptr = buf;
        remain = RX_BUFFER_LEN;
      }

      if (!initial) {
        byte wait = flash_page_wait;
        while (--wait) { longWait(); }
        byte wait_ret = flashWaitOperation(FLASH_PAGE_WRITE_EXPECTED_STATUS);
        if (wait_ret != FLASH_PAGE_WRITE_EXPECTED_STATUS) {
          sendErrorReport(bank, --address, wait_ret, FLASH_PAGE_WRITE_EXPECTED_STATUS);
          break;
        }
      }

      writebyte_cart2(flash_banks[0], flash_address[0], flash_cmd[0]);
      writebyte_cart2(flash_banks[1], flash_address[1], flash_cmd[1]);
      writebyte_cart2(flash_banks[2], flash_address[2], flash_cmd[2]);

      page_count = flash_page_size;
      initial = false;
    }

    // output program byte
    byte b = *bufptr;
    writebyte_cart2(bank, address, b);

    address++;
    bufptr++;

  } while (--datasize);
}

void flashWriteCart() {
  //コマンド受信
  while (Serial.available() < 5)
    ;

  word address = Serial_readWord();
  byte bank = Serial.read();
  word datasize = Serial_readWord();

  flashBulkWriteInit();
  if (flash_byte_verify) {
    flashByteProgram(bank, address, datasize);
  } else {
    flashPageProgram(bank, address, datasize);
  }

  flashBulkWriteExit();

  //Send End Signal
  serial_send('E');
  serial_send((byte)(max_loop));
  serial_send((byte)(max_loop >> 8));
  max_loop = FLASH_WAIT_TIMEOUT_CYCLE;
  //  Serial.println("WRITE_END");
}

#pragma GCC pop_options
