# HongKongArduinoClone
「こーどねーむホンコン with Arduino」用高速化ファーム＋クライアント  
 SNES Cartridge Dumper with Arduino UNO

## 概要 / about
このプログラムは、たにやま氏開発の"こーどねーむ「ホンコン」 with Arduino"に手を加えて高速化・SRAMの吸出しなどの機能を加えたものです。Windows専用です。  
This project is mods of "Codename Hong kong with Arduino". It is SNES cartridge dumper. Original has been developed by Taniyama. This project adds high speed damping, SRAM read/write and other. Only works on windows NT and later.
  
こーどねーむ「ホンコン」 with Arduino  
<http://hongkongarduino.web.fc2.com/>
	
## 開発環境 / Dev Environment
	OS : Windows 7 32bit & 64bit
	Arduino : Aitendo製のArduino UNO互換機（ATMega328P 16MHz）
	ArduinoIDE : ver1.6.6
	DevLang：ActiveBasic ver4 + 自分用ライブラリ
	
	コンパイルにはこちらのライブラが必要です。
	https://github.com/RGBA-CRT/RGBALib


## リンク / Link
こーどねーむ「ホンコン」 with Arduino / Original page  
<http://hongkongarduino.web.fc2.com>

たにやま氏のリポジトリ / Original repository  
<https://github.com/SusumuTaniyama/HongKongArduino>

このプログラムについてのページ / My page  
<http://rgbacrt.seesaa.net/article/435543541.html>

### 参考コード・資料 / Referenced documents
 * 	[http://hongkongarduino.web.fc2.com/archive]  
 * 	[https://github.com/sanni/cartreader/]  
 * 	[http://problemkaputt.de/fullsnes.txt]  

## ダウンロード / Download
<https://github.com/RGBA-CRT/HongKongArduinoClone/releases>


## 特殊カートリッジについて / Special carts
対応状況は以下の通りです。
 * [NG] SFメモリカセット
 * [OK] 特殊LoROM (ダビスタ96)
 * [NG] 8Mメモリパック
 * [OK] S-DD1
 * [OK] SA-1
 * [OK] SPC7110
 * [未確認] ExHiROM
 * [未確認] CX4
 * [未確認] STシリーズ
 
SA-1とSPC7110は以下のクロックモジュール回路を追加すると吸い出せるようになります。  
Dump from SA-1 and SPC7110 needs following circuits.  
![回路図](https://raw.githubusercontent.com/RGBA-CRT/HongKongArduinoClone/master/SA1.png "回路図")   
![SS](https://raw.githubusercontent.com/RGBA-CRT/HongKongArduinoClone/master/SA1SS.png "SS")  　　　  　

## 履歴 / History
	[2016/03/23]ver 0.0 - 初回リリース
	[2016/07/20]ver 0.1 - チェックサム判定機能追加
	[2017/02/17]ver 0.3 - 最適化＆バグフィックス、ファーム判定機能追加
	[2017/09/17]ver 0.4 - S-DD1に対応
	[2017/09/19]ver 0.5 - SPC7110に対応
	[2017/12/24]ver 0.6 - ダビスタ96(特殊LoROM)に対応, HiROM 256Kbit SRAMに対応
	[2018/02/10]ver 0.62- UIを微修正。D&DでROMヘッダを表示するようにした。
