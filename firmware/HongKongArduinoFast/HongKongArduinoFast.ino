//こーどねーむ「ホンコン」 with Arduino 高速版ファームウェア
//Original : たにやま 2016/2/14 [http://hongkongarduino.web.fc2.com/]
//Optimize : RGBA_CRT 2016/3/19 [rgba3crt1p@gmail.com]
//2016/7/2 update

//[HongKongArduinoFast.hex]は-O3でコンパイルされたバイナリです。
//通常は↑を書き込んで使ってください

/*
	Reference :
		'r' or 'R' の引数はアドレス(3バイト)とデーターサイズ(3バイト)で、
							 指定したアドレスからデーターサイズ分を送信します。

		'w' or 'W' の引数はアドレス(3バイト)とデーターサイズ(3バイト)で、
							 アドレスをインクリメントしながら書き込んでいきます。

		'a' or 'A' の引数はアドレス(３バイト)で、アドレスバスを操作できます。

		'c'　の引数はフラグ（１バイト）で、/CE /OE /WE RESET等を操作します。

*/

//データバス[PORTD]
const int DATA0 = 2;
const int DATA1 = 3;
const int DATA2 = 4;
const int DATA3 = 5;
const int DATA4 = 6;
const int DATA5 = 7;

//[PORTB]
const int DATA6 = 8;
const int DATA7 = 9;

//74HCシリーズの制御
const int GD = 10;
const int G0 = 11;
const int G1 = 12;
const int G2 = 13;

//コントロールピン
const int DIR = 14;
const int CK = 15;
const int OE = 16;
const int CS = 17;
const int WE = 18;
const int RST = 19;

//データピンの方向設定
inline void setDataDir(int DATADIR)
{
  /*for (int i = 0; i < 8; i++) {
  		pinMode(DATA0 + i, DATADIR);
  	}
  */
  if (DATADIR == INPUT) {
    DDRD &= 0b00000011;
    DDRB &= 0b11111100;
  } else {
    DDRD |= 0b11111100;
    DDRB |= 0b00000011;
  }
}/*

  //データバスの方向設定
  inline void setDataBusDir(int DATADIR)
  {
	if (DATADIR == INPUT) {
		DDRD &= 0b00000011;
		DDRB &= 0b11111100;
		digitalWrite(DIR, LOW); //	Arduino <= [A_Bus_OUT]-[B_Bus_IN] <= SFC
		digitalWrite(GD, LOW); // 74HC245 Enable

	} else {
		DDRD |= 0b11111100;
		DDRB |= 0b00000011;
		digitalWrite(DIR, HIGH); // Arduino => [A_Bus_IN]-[B_Bus_OUT] => SFC
		digitalWrite(GD, LOW); // 74HC245 Enable
	}

  }*/

//データーバスへ値をセット
inline void setData(byte b)
{
  /* for (int i = 0; i < 8; i++) {
  		 digitalWrite(DATA0 + i, (b & (1 << i)) ? HIGH : LOW);
  	}
  */
  PORTD &= 0b00000011;
  PORTD |= b << 2;
  PORTB &= 0b11111100;
  PORTB |= b >> 6;
}

//アドレスバスを構成する74HC377へ値をセット
inline void outCh(int ch, byte b)
{
  //digitalWrite(G0 + ch, LOW); // Ch Enable
  PORTB &= ~0b00001000 << ch; //0b11110111

  //setData(b);
  PORTD &= 0b00000011;
  PORTD |= b << 2;
  PORTB &= 0b11111100;
  PORTB |= b >> 6;

  // digitalWrite(CK, HIGH);
  PORTC |= 0b00000010;

  //digitalWrite(CK, LOW);
  PORTC &= 0b11111101;

  //digitalWrite(G0 + ch, HIGH); // Ch Disable
  PORTB |= 0b00001000 << ch;
}

byte lastadr[3];
//アドレスバスを設定
inline void setAddress(unsigned long address, int isLoROM)
{
  if (isLoROM) {
    //unsigned long upper = address / 0x8000;
    //unsigned long lower = address % 0x8000;
    address = address / 0x8000 * 2 * 0x8000 + address % 0x8000 + 0x8000;
  }

  /*
  	byte a[3];

  	a[0] = (byte)(address >> (8 * 0));
  	a[1] = (byte)(address >> (8 * 1));
  	a[2] = (byte)(address >> (8 * 2));

  	outCh(0, a[0]);
  	outCh(1, a[1]);
  	outCh(2, a[2]);
  */

  /* outCh(0, address >> (8 * 0));
    outCh(1, address >> (8 * 1));
    outCh(2, address >> (8 * 2));
  */
  byte spritAdr = address;
  if (lastadr[0] != spritAdr) {
    outCh(0, spritAdr);
    lastadr[0] = spritAdr;
  }

  spritAdr = address >> (8);
  if (lastadr[1] != spritAdr) {
    outCh(1, spritAdr);
    lastadr[1] = spritAdr;
  }

  spritAdr = address >> (16);
  if (lastadr[2] != spritAdr) {
    outCh(2, spritAdr);
    lastadr[2] = spritAdr;
  }


}
#define Disable74HC245() PORTB |= 0b00000100
#define Enable74HC245() digitalWrite(GD, LOW)
void readData()
{
  setDataDir(INPUT);

  //PWM対応ポートの場合ただPORTBをいじってもダメらしい
  Enable74HC245();
  //digitalWrite(GD, LOW); // Ch Enable
  //turnOffPWM2(digitalPinToTimer(GD));
  //PORTB &= 0b11111011;

  byte b = (PIND >> 2) | ((PINB << 6));
  /*if (PIND & 0b00000100)			 b |= (1 << 0);	//if (digitalRead(DATA0 + 0) == HIGH)			 b |= (1 << 0);
  	if (PIND & 0b00001000)			 b |= (1 << 1);
  	if (PIND & 0b00010000)			 b |= (1 << 2);
  	if (PIND & 0b00100000)			 b |= (1 << 3);
  	if (PIND & 0b01000000)			 b |= (1 << 4);
  	if (PIND & 0b10000000)			 b |= (1 << 5);
  	if (PINB & 0b00000001)			 b |= (1 << 6);	// if (digitalRead(DATA0 + 6) == HIGH)			 b |= (1 << 6);
  	if (PINB & 0b00000010)			 b |= (1 << 7);	// if (digitalRead(DATA0 + 7) == HIGH)			 b |= (1 << 7);
  */

  //PORTB |= 0b00000100; // digitalWrite(GD, HIGH); // Ch Disable
  Disable74HC245();
  setDataDir(OUTPUT);

  Serial.write(b);
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


void setup()
{
  setDataDir(OUTPUT);

  for (int i = GD; i <= RST; i++) {
    pinMode(i, OUTPUT);
  }

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

  //アクセスランプを消灯
  setAddress(0x000000, 0);

  // Serial.begin(115200);
  Serial.begin( 500000);
}

void loop()
{
  if (Serial.available() >= 2) {
    byte cmd = Serial.read();

    if (cmd == 'R' || cmd == 'r') {
      while (Serial.available() < 6) {
        delay(1);
      }

      int isLoROM = false;
      if (cmd == 'r') {
        isLoROM = true;
      }
      unsigned long address = Serial.read();;
      address += ((unsigned long)Serial.read() << 8);
      address += ((unsigned long)Serial.read() << 8 * 2);

      unsigned long datasize = Serial.read();
      datasize += ((unsigned long)Serial.read() << 8);
      datasize += ((unsigned long)Serial.read() << 8 * 2);
      /*     unsigned long a = Serial.read();
           unsigned long address = a;
           a = Serial.read();
           address += (a << 8);
           a = Serial.read();
           address += (a << 8 * 2);

           a = Serial.read();
           unsigned long datasize = a;
           a = Serial.read();
           datasize += (a << 8);
           a = Serial.read();
           datasize += (a << 8 * 2);*/

      unsigned long goalAdr = address + datasize;
      while (address < goalAdr) {
        setAddress(address, isLoROM);
        readData();
        address++;
      }

      /*   } else if (cmd == 'd') {
           // databus
           byte b = Serial.read();
           setDataBusDir(OUTPUT);
           setData(b);*/

    } else if (cmd == 'e') {
      while (1) {
        while (Serial.available() >= 1)
          Serial.write(Serial.read());
      }

    } else if (cmd == 'c') {
      // control
      byte b = Serial.read();

      digitalWrite(OE , (b & B0001) ? HIGH : LOW);
      digitalWrite(CS , (b & B0010) ? HIGH : LOW);
      digitalWrite(WE , (b & B0100) ? HIGH : LOW);
      digitalWrite(RST, (b & B1000) ? HIGH : LOW);

    } else if (cmd == 'a' || cmd == 'A') {
      while (Serial.available() < 3)	delay(1);

      int isLoROM = false;
      if (cmd == 'a') isLoROM = true;
      unsigned long address = Serial.read();;
      address += ((unsigned long)Serial.read() << 8);
      address += ((unsigned long)Serial.read() << 8 * 2);

      /*
            unsigned long a = Serial.read();
            unsigned long address = a;
            a = Serial.read();
            address += (a << 8);
            a = Serial.read();
            address += (a << 8 * 2);*/

      setAddress(address, isLoROM);

    } else if (cmd == 'W' || cmd == 'w') {
      int isLoROM = false;
      if (cmd == 'w') {
        isLoROM = true;
      }
      writeSRAM(isLoROM);

    }
  }
}

