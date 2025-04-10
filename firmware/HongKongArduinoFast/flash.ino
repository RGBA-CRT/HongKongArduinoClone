#pragma GCC push_options
#pragma GCC optimize("O3")

//flash config
#define FLASH_COMMAND_LENGTH 3
static byte flash_bank;
static word flash_address[FLASH_COMMAND_LENGTH];  // = {0xAAAA,0x5555,0xAAAA}
static byte flash_cmd[FLASH_COMMAND_LENGTH];      // = {0xAA  ,0x55,  [cmd] }

void flashReceiveConfig() {
  // flash_bank + (flash_address,flash_cmd) * FLASH_COMMAND_LENGTH
  while (Serial.available() < (1 + FLASH_COMMAND_LENGTH * 3))
    ;
  flash_bank = Serial.read();
  for (byte i = 0; i < FLASH_COMMAND_LENGTH; i++) {
    flash_address[i] = Serial_readWord();
    flash_cmd[i] = Serial.read();
  }
}

// 500usぐらいをtimeoutとしたい
// clockは16MHz, 1cycle1命令とする(今もそうなのか？). 62.5ns
// readDataが108clkぐらい
// その他関数内のループを少なく見積もって5clkぐらい。7,062.5ns
#define FLASH_WAIT_TIMEOUT_CYCLE 140
inline bool flashWaitOperation(byte expect_byte){
  for(byte i = FLASH_WAIT_TIMEOUT_CYCLE; i; --i){
    if(readData() == expect_byte){
      return false;
    }
  }
  return true;
}

void flashWriteCart() {
  //コマンド受信
  while (Serial.available() < 5)
    ;
  word address = Serial_readWord();
  byte bank = Serial.read();
  word datasize = Serial_readWord();

  CART_WRITE_DISABLE();
  CART_OUTPUT_DISABLE();
  BB_DIR_OUTPUT();

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
    writebyte_cart(flash_bank, flash_address[0], flash_cmd[0]);
    writebyte_cart(flash_bank, flash_address[1], flash_cmd[1]);
    writebyte_cart(flash_bank, flash_address[2], flash_cmd[2]);
#endif

    // output program byte
    byte b = *bufptr;
    writebyte_cart(bank, address, b);
    if(flashWaitOperation(b)){
      // report fail to write
      serial_send('X');
      break;
    }
  
    address++;
    bufptr++;
  } while( --datasize );

  CART_WRITE_DISABLE();
  BB_OUT_DISABLE();
  BB_DIR_INPUT();

  //Send End Signal
  serial_send('E');
  //  Serial.println("WRITE_END");
}

#pragma GCC pop_options
