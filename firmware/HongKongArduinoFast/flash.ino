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
static byte flash_bank;
static word flash_address[FLASH_COMMAND_LENGTH];  // = {0xAAAA,0x5555,0xAAAA}
static byte flash_cmd[FLASH_COMMAND_LENGTH];      // = {0xAA  ,0x55,  [cmd] }

#define FLASH_CONFIG_CEOE_SWAP 0x01

void flashReceiveConfig() {
  // flash_bank + (flash_address,flash_cmd) * FLASH_COMMAND_LENGTH
  while (Serial.available() < (1 + FLASH_COMMAND_LENGTH * 3 + 1))
    ;
  flash_bank = Serial.read();
  for (byte i = 0; i < FLASH_COMMAND_LENGTH; i++) {
    flash_address[i] = Serial_readWord();
    flash_cmd[i] = Serial.read();
  }
  uint8_t flags = Serial.read();
  SetFlashOECtrl((flags & FLASH_CONFIG_CEOE_SWAP));
}

inline void bulkReadInit() {
  setDataPin(0x00);
  dataDirInput();
  BB_DIR_INPUT();
}
inline void bulkReadExit() {
  BB_DIR_TOGGLE();
  dataDirOutput();
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

  // tAS: Address Setup Time: アドレスを出力してからWEを下げていいまで: 0ns @ S29GL032
  // tAH: Address Hold Time: WEが下がってからアドレスを潰していいまで: 45ns
  __asm__ volatile("nop"); // 100ns
  __asm__ volatile("nop"); // 200ns
  // longWait();

  CART_WRITE_TOGGLE();

  // tDS: data setup time: データが出てからWEが立ち上がるまで: 35ns @ S29GL032
  // tDH: Data Hold Time: WE立ち上がってから潰して良いまで: 0ns @ S29GL032
  // tWP: WEパルスの幅: 35ns @ S29GL032

  __asm__ volatile("nop");
  __asm__ volatile("nop");
  // nop2個で200nsぐらい（多分）

  CART_WRITE_TOGGLE();
  __asm__ volatile("nop"); // 300ns
}

void flashWriteCart() {
  //コマンド受信
  while (Serial.available() < 5)
    ;
  word address = Serial_readWord();
  byte bank = Serial.read();
  word datasize = Serial_readWord();

  flashBulkWriteInit();

  // メモリは余っているので速度優先でじゃんじゃんつかおう
  byte* bufptr = buf;
  word remain = 1;  // 最初は必ず受信させる

  do {
    //データ受信
    if (!(--remain)) {
      hostsync_receive(RX_BUFFER_LEN);
      bufptr = buf;
      remain = RX_BUFFER_LEN;
    }

#if 0
    // Flash command: PROGRAM
    for (byte i = 0; i < FLASH_COMMAND_LENGTH; i++) {
      writebyte_cart(flash_bank, flash_address[i], flash_cmd[i]);
    }
#else
    writebyte_cart2(flash_bank, flash_address[0], flash_cmd[0]);
    writebyte_cart2(flash_bank, flash_address[1], flash_cmd[1]);
    writebyte_cart2(flash_bank, flash_address[2], flash_cmd[2]);
#endif

    // output program byte
    byte b = *bufptr;
    writebyte_cart2(bank, address, b);

    address++;
    bufptr++;

    byte wait_ret = flashWaitOperation(b);
    if (wait_ret != b) {
      // report fail to write
      --address;
      serial_send('X');
      serial_send((byte)address);
      serial_send((byte)(address >> 8));
      serial_send(bank);
      serial_send(b);
      serial_send(wait_ret);
      serial_send('X');
      break;
    }

  } while (--datasize);

  flashBulkWriteExit();

  //Send End Signal
  serial_send('E');
  serial_send((byte)(max_loop));
  serial_send((byte)(max_loop >> 8));
  max_loop = FLASH_WAIT_TIMEOUT_CYCLE;
  //  Serial.println("WRITE_END");
}

#pragma GCC pop_options
