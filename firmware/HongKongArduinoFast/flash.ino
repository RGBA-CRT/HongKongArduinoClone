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
// static uint8_t flash_bank;
static uint8_t flash_banks[FLASH_COMMAND_LENGTH];
static uint16_t flash_address[FLASH_COMMAND_LENGTH];  // = {0xAAAA,0x5555,0xAAAA}
static uint8_t flash_cmd[FLASH_COMMAND_LENGTH];      // = {0xAA  ,0x55,  [cmd] }
static uint8_t flash_byte_verify;
static uint8_t flash_page_size;
static uint8_t flash_page_wait;
static uint8_t flash_verify_fixed_value;
static uint8_t flash_verify_byte;
#define FLASH_PAGE_WRITE_EXPECTED_STATUS 0x80

void flashReceiveConfig() {
  // flash_bank + (flash_address,flash_cmd) * FLASH_COMMAND_LENGTH
  while (serial_available() < (FLASH_COMMAND_LENGTH * 4))
    ;
  // flash_bank = serial_read();
  for (uint8_t i = 0; i < FLASH_COMMAND_LENGTH; i++) {
    flash_banks[i] = serial_read();
    flash_address[i] = serial_read_word();
    flash_cmd[i] = serial_read();
  }

  while (serial_available() < 4)
    ;
  gflags = serial_read();
  SetFlashOECtrl((gflags & FLASH_CONFIG_CEOE_SWAP));
  flash_byte_verify = gflags & FLASH_CONFIG_BYTE_VERIFY;
  flash_verify_fixed_value = gflags & FLASH_CONFIG_VERIFY_FIXED_VALUE;

  flash_page_size = serial_read();
  flash_page_wait = serial_read();
  flash_verify_byte = serial_read();

#if 0
  Serial.print("FMT25112301\n");
  for (uint8_t i = 0; i < FLASH_COMMAND_LENGTH; i++) {
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

inline uint8_t bulkReadAcquire() {
  FLASH_OE_TOGGLE();

  // tCE: CEがLowになってから有効データ出力までの時間: 70ns@LV640
  // tOE: OEがLowになってから有効データ出力までの時間: 30ns@LV640
  __asm__ volatile("nop");
  __asm__ volatile("nop");

  uint8_t b = getDataPin();

  FLASH_OE_TOGGLE();

  // tOEh: OEをHighにしてほしい時間: 10ns@LV640
  // __asm__ volatile("nop"); // 後続の条件分岐で十分に待ってると思う。

  return b;
}


// 500usぐらいをtimeoutとしたい
// clockは16MHz, 1cycle1命令とする(今もそうなのか？). 62.5ns
// readDataが108clkぐらい
// その他関数内のループを少なく見積もって5clkぐらい。7,062.5ns
#define FLASH_WAIT_TIMEOUT_CYCLE 0xffff
// #define POLL_ONLY_DQ7

uint16_t max_loop = FLASH_WAIT_TIMEOUT_CYCLE;
uint8_t flashWaitOperation(uint8_t expect_byte) {
  bulkReadInit();
  uint8_t ret;
  uint8_t ok_cnt = 2;
  uint16_t i = FLASH_WAIT_TIMEOUT_CYCLE;

  // tBusy: 70ns@LV640
  // S29L032N tBusy
  // __asm__ volatile("nop");
  // __asm__ volatile("nop");
  __asm__ volatile("nop");

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
void writebyte_cart2(uint8_t bank, uint16_t address, uint8_t data) {
  setAddressFFs(bank, address);
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

void sendErrorReport(uint8_t bank, uint16_t address, uint8_t real_byte, uint8_t expected_byte) {
  serial_send('X');
  serial_send((byte)address);
  serial_send((byte)(address >> 8));
  serial_send(bank);
  serial_send(expected_byte);
  serial_send(real_byte);
  serial_send('X');
}

#define GetProgDoneResponce(x) (flash_verify_fixed_value ? flash_verify_byte : x)

void flashByteProgram(uint8_t bank, uint16_t address, uint16_t datasize) {
  // メモリは余っているので速度優先でじゃんじゃんつかおう
  byte* bufptr = large_rx_buf;
  uint16_t remain = 1;  // 最初は必ず受信させる
  uint8_t page_count = flash_page_size;

  do {
    //データ受信
    if (!(--remain)) {
      hostsync_receive(LARGE_RX_BUFFER_LEN);
      bufptr = large_rx_buf;
      remain = LARGE_RX_BUFFER_LEN;
    }

    writebyte_cart2(flash_banks[0], flash_address[0], flash_cmd[0]);
    writebyte_cart2(flash_banks[1], flash_address[1], flash_cmd[1]);
    writebyte_cart2(flash_banks[2], flash_address[2], flash_cmd[2]);

    // output program byte
    uint8_t b = *bufptr;
    writebyte_cart2(bank, address, b);

    address++;
    bufptr++;

    uint8_t expect = GetProgDoneResponce(b);
    uint8_t wait_ret = flashWaitOperation(expect);
    if (wait_ret != expect) {
      sendErrorReport(bank, --address, wait_ret, expect);
      break;
    }
  } while (--datasize);
}

void flashPageProgram(uint8_t bank, uint16_t address, uint16_t datasize) {
  // メモリは余っているので速度優先でじゃんじゃんつかおう
  byte* bufptr = large_rx_buf;
  uint16_t remain = flash_page_size;  // 最初は必ず受信させる
  uint8_t page_count = 1;            // 最初は必ずページ跨ぎ処理（コマンド発行）をする
  bool initial = true;

  do {
    if (!(--page_count)) {
      //データ受信
      remain -= flash_page_size;
      if (!remain) {
        hostsync_receive(LARGE_RX_BUFFER_LEN);
        bufptr = large_rx_buf;
        remain = LARGE_RX_BUFFER_LEN;
      }

      if (!initial) {
        uint8_t wait = flash_page_wait;
        while (--wait) { longWait(); }
        uint8_t wait_ret = flashWaitOperation(FLASH_PAGE_WRITE_EXPECTED_STATUS);
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
    uint8_t b = *bufptr;
    writebyte_cart2(bank, address, b);

    address++;
    bufptr++;

  } while (--datasize);
}

void flashWriteCart() {
  //コマンド受信
  while (serial_available() < 5)
    ;

  uint16_t address = serial_read_word();
  uint8_t bank = serial_read();
  uint16_t datasize = serial_read_word();

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
