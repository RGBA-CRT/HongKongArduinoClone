#pragma GCC push_options
#pragma GCC optimize("Os")

#ifdef ENABLE_ST018_BIOS_DUMP
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
#endif

#pragma GCC pop_options

