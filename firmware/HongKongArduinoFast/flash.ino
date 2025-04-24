#pragma GCC push_options
#pragma GCC optimize("O3")


#define FLASH_OE_ENABLE()   PORTC &= val_oe_and_mask
#define FLASH_OE_DISABLE()  PORTC |= val_oe_or_mask
#define FLASH_OE_TOGGLE()   PINC  =  val_oe_or_mask

#define FLASH_CE_ENABLE()   PORTC &= ~val_ce_or_mask
#define FLASH_CE_DISABLE()  PORTC |= val_ce_or_mask
#define FLASH_CE_TOGGLE()   PINC  =  val_ce_or_mask

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

inline void bulkReadInit(){
  setDataPin(0x00);
  dataDirInput();
  BB_DIR_INPUT();
}
inline void bulkReadExit(){
  BB_DIR_TOGGLE();
  dataDirOutput();
}
inline byte bulkReadAcquire()
{
  FLASH_OE_TOGGLE();
  
  __asm__ volatile("nop");

  byte b = getDataPin();

  FLASH_OE_TOGGLE();
  // __asm__ volatile(
  //   "nop\n"
  // );

  return b;
}


// 500usぐらいをtimeoutとしたい
// clockは16MHz, 1cycle1命令とする(今もそうなのか？). 62.5ns
// readDataが108clkぐらい
// その他関数内のループを少なく見積もって5clkぐらい。7,062.5ns
#define FLASH_WAIT_TIMEOUT_CYCLE 140
// #define POLL_ONLY_DQ7

// return: error(true) or ok(false)
inline bool flashWaitOperation(byte expect_byte){
  bool ret = false;
  bulkReadInit();

  for(byte i = FLASH_WAIT_TIMEOUT_CYCLE; i; --i){
    if(bulkReadAcquire() == expect_byte){
      goto fwoExit;
    }
  }
  ret = true;

fwoExit:
  bulkReadExit();
  return ret;
}

void flashBulkWriteInit(){
  // 初期PIN状態。ループ内最適化のためtoggleなどしているため注意。
  CART_WRITE_DISABLE();
  FLASH_OE_DISABLE();
  BB_DIR_OUTPUT();
  BB_OUT_ENABLE();
  FLASH_CE_ENABLE();

}

void flashBulkWriteExit(){
  FLASH_CE_DISABLE();
  CART_WRITE_DISABLE();
  FLASH_OE_DISABLE();
  BB_OUT_DISABLE();
  BB_DIR_INPUT();
}

//　/WRとかをちゃんと制御して書き込む
inline void writebyte_cart2(byte bank, word address, byte data) {
  setAddress_(bank, address);
  setDataPin(data);

  // __asm__ volatile("nop");
  // __asm__ volatile("nop");

  CART_WRITE_TOGGLE();

  // __asm__ volatile("nop");
  // __asm__ volatile("nop");

  CART_WRITE_TOGGLE();
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
  word remain = 1; // 最初は必ず受信させる

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

    if(flashWaitOperation(b)){
      // report fail to write
      serial_send('X');
      break;
    }
  
  } while( --datasize );

  flashBulkWriteExit();

  //Send End Signal
  serial_send('E');
  //  Serial.println("WRITE_END");
}

#pragma GCC pop_options
