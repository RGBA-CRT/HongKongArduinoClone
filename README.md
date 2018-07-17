# HongKongArduinoClone
「こーどねーむホンコン with Arduino」用高速化ファーム＋クライアント  
 SNES Cartridge Dumper with Arduino UNO highspeed firmware + host program

## 概要 / about
このプログラムは、たにやま氏開発の"こーどねーむ「ホンコン」 with Arduino"に手を加えて高速化・SRAMの吸出しなどの機能を加えたものです。Windows用です。Wineでも動作します。  
  
This project is mods of "Codename Hong kong with Arduino". It is SNES cartridge dumper. Original has been developed by Taniyama.   
This project adds high speed damping, SRAM read/write and other. Only works on windows NT or wine.　　
  
こーどねーむ「ホンコン」 with Arduino / original page  
<http://hongkongarduino.web.fc2.com/>
	
## 開発環境 / Dev Environment
	OS : Windows 7
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


## 特殊カートリッジについて / About Special Carts
対応状況は以下の通りです。
 * [OK] SFメモリカセット / NP FLASH Cart(SF Memory)
 * [OK] 特殊LoROM (ダビスタ96) / Special LoROM(3MB)
 * [OK] 8Mメモリパック / satellaview 8M data pack
 * [OK] S-DD1
 * [OK] SA-1
 * [OK] SPC7110
 * [OK] ExHiROM
 * [NG] CX4
 * [NG] STシリーズ
 
SA-1とSPC7110は以下のクロックモジュール回路を追加すると吸い出せるようになります。  
Dump from SA-1 and SPC7110 needs following Clock Modlue.  
![回路図](https://raw.githubusercontent.com/RGBA-CRT/HongKongArduinoClone/master/ss/SA1.png "回路図")   
![SS](https://raw.githubusercontent.com/RGBA-CRT/HongKongArduinoClone/master/ss/SA1SS.png "SS")  　　　  　

## SFメモリカセットについて / About NP FLASH Cart
Ver0.8からSFメモリカセットの吸出しに対応しました。

![NPSS](https://raw.githubusercontent.com/RGBA-CRT/HongKongArduinoClone/master/ss/NP.png "NP")  　　

 * 接続手順
	 1. クロックモジュールを装着する。まだカセットは挿入しない。※1
	 1. PC側でConnectボタンを押す。
	 1. カセットを挿入し、GetCartInfoを押す。
	 1. MENU PROGRAMと出れば成功。・・・・や888888等と表示された場合は1.からやり直し。(割と失敗が多い)
	 1. Mappingが"SF MEMORY MX15001"となっているときは、SFMボタンが出現するので押す。するとスクショのようなダイアログが出る。
	 1. Ditect SFM2ボタンを押す。するとGame Listに入っているゲームが表示される。出なかったら1.からやり直し。
	 1. 吸い出す。  
	 ※1 カセットを入れたままConnectをすると、高確率で失敗するので注意。

 * ゲームの吸出し
 
	 * 吸い出したいゲームを選択し、Dump Gameボタンを押す。
	 * Full Dumpを吸い出すと、ゲーム部分が全て連結された状態で吸い出される。
	 * ウィンドウ本体の方のDumpROMボタンを使うと正しい容量で吸い出せないので、Dump Gameボタンを使ってください。

 * SRAMの吸出し
	  * 吸い出したいゲームを選択し、Switch Gameボタンを押す。
	  * ウィンドウの本体のDumpSRAMボタンを押してSRAMを吸い出す。
 
---
 * Detection of NP
	 1. Install Clock Module. Don't insert cart yet.
	 1. Press Connect button.
	 1. Insert NP cartridge and Press GetCartInfo.
	 1. If GameTitle isn't "MENU PROGRAM" it is an error. Retry from 1.
	 1. When Mapping is SF MEMORY MX15001, be displayed SFM button. Press it. Appear the Window like ScreenShot.
	 1. Press Ditect SF2. Then the game in Game List is displayed. If error, go to 1
	 1. Dump

 * Dumping the Game
 
	 * Select the game and press Dump Game.
	 * If use the main window's DumpROM button, you can not dump with the correct size.

 * Dumping the SRAM
	  * Select the game and press Switch Game.
	  * Press main window's DumpSRAM button.
 

## 履歴 / History
	[2016/03/23]ver 0.0 - 初回リリース
	[2016/07/20]ver 0.1 - チェックサム判定機能追加
	[2017/02/17]ver 0.3 - 最適化＆バグフィックス、ファーム判定機能追加
	[2017/09/17]ver 0.4 - S-DD1に対応
	[2017/09/19]ver 0.5 - SPC7110に対応
	[2017/12/24]ver 0.6 - ダビスタ96(特殊LoROM)に対応, HiROM 256Kbit SRAMに対応
	[2018/02/10]ver 0.62- UIを微修正。D&DでROMヘッダを表示するようにした。
	[2018/02/26]ver 0.7 - ファームウェアアップデート。動的ボーレート, 速度向上(55 -> 96KB/s)
	[2018/02/26]ver 0.8 - SFメモリカセットに対応。チェックサム計算を修正
	[2018/07/18]ver 0.9 - BS-Xメモリーパックに対応。
