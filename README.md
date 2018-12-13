# HongKongArduinoClone
 「こーどねーむホンコン with Arduino」を高速化するスケッチとホストプログラム  
SNES Cartridge Dumper with Arduino UNO highspeed firmware + host program  

![SS](https://raw.githubusercontent.com/RGBA-CRT/HongKongArduinoClone/master/ss/SA1SS.png "SS")  　

## 概要 / About
+ このプログラムは、たにやま氏の"こーどねーむ「ホンコン」 with Arduino"に手を加え、  
  約40倍の高速化・特殊カートリッジ対応しなどの機能を加えたものです。
+ Arduino UNOとWindows PCでスーファミのカセットの吸出しができます。
+ This project is mods of "Codename Hong Kong with Arduino".   
  It is SNES cartridge dumper. Original has been developed by Taniyama.   
+ This project supports high-speed dumping, SRAM read/write, BS-X Memory Pack and other.  　　  　

## リンク / Link
+ [こーどねーむ「ホンコン」 with Arduino / Original page](http://hongkongarduino.web.fc2.com)
+ [たにやま氏のリポジトリ / Original repository](https://github.com/SusumuTaniyama/HongKongArduino)
+ [このプログラムについてのページ / My page](http://rgbacrt.seesaa.net/article/435543541.html)


## 参考資料 / Referenced documents
 * <http://hongkongarduino.web.fc2.com/archive>  
 * <https://github.com/sanni/cartreader/> 
 * <http://problemkaputt.de/fullsnes.txt>  


## 対応カートリッジについて / Surpported Carts
対応状況は以下の通りです。
 * [OK] LoROM
 * [OK] HiROM
 * [OK] SuperFX
 * [OK] DSP1
 * [OK] SFメモリカセット / NP FLASH Cart(SF Memory)
 * [OK] 特殊LoROM (ダビスタ96) / Special LoROM(3MB)
 * [OK] 8Mメモリパック / Satellaview 8M data pack
 * [OK] ExHiROM
 * [OK] S-DD1
 * [OK] SA-1
 * [OK] SPC7110
 * [NG] CX4
 * [NG] ST01x
  
SA-1とSPC7110は以下のクロックモジュール回路を追加すると吸い出せるようになります。
認識率が低いので認識するまでリトライしてください。ArduinoにACアダプタをつけると安定性が向上します。  
Dump from SA-1 and SPC7110 needs following Clock Modlue.  
![回路図](https://raw.githubusercontent.com/RGBA-CRT/HongKongArduinoClone/master/ss/SA1.png "回路図")   

## 動作環境 / System requirements
+ Windows 98SE, Windows 2000 or later, Wine
+ Arduino UNO(ATmega 328P), Arduino NANO(ATmega168)
+ ATmega168の場合、現在のバージョンではｸﾛｯｸﾓｼﾞｭｰﾙの制御プログラムが収まりません。
+ Recommend Serial Converter : CH340


## ダウンロード / Download
<https://github.com/RGBA-CRT/HongKongArduinoClone/releases>

---

## SFメモリカセットについて / About NP FLASH Cart
+ [接続手順](https://github.com/RGBA-CRT/HongKongArduinoClone/wiki/SF-Memory)

## SPC7110の注意 / SPC7110 Warning
+ SPC7110のカートリッジ(天外魔境ZEROなど)は特定の手順で接続しないと、セーブが消えます。詳しくは以下から。
+ If you don't connect the cartridge of SPC7110 (Far East of Eden Zero) in a special procedure,
  Save RAM will break. For details, please see below.
+ <https://github.com/RGBA-CRT/HongKongArduinoClone/wiki/SPC7110-SRAM-Read-Write>

## 既知の問題 / Known issues
+ 問題が起きたときに読んでください
+ [click here](https://github.com/RGBA-CRT/HongKongArduinoClone/wiki/Known-Issues)

## 開発環境 / Dev Environment
+ OS : Windows 7 x64
+ Arduino : Aitendo製のArduino UNO互換機（ATMega328P 16MHz
+ ArduinoIDE : ver1.6.6
+ Language：[ActiveBasic4](https://www.activebasic.com/) + [RGBALib](https://github.com/RGBA-CRT/RGBALib)

## ライセンス / Licenses
+ たにやま氏制作部分（HongKongArduinoFast.ino）にはクリエイティブコモンズby-ncが適用されます。
[![cc](https://licensebuttons.net/l/by-nc/4.0/88x31.png)](https://creativecommons.org/licenses/by-nc/4.0/)  

+ 私が制作した部分(Windowsプログラム,　HongKongArduinoFast.ino以外のファイル)はCC0が適応されます。
[![cc0](https://licensebuttons.net/p/zero/1.0/80x15.png)](http://creativecommons.org/publicdomain/zero/1.0/)  

---

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
    [2018/07/18]ver 0.9 - SA-1追加回路修正, サテラビューに対応
    [2018/08/24]ver 1.0 - SRAMリードライトを改善, UIを改善, 正式版
    [2018/08/24]ver 1.0b- SA1の認識率向上

----
Programmed by RGBA_CRT 2018  
Project url: https://github.com/RGBA-CRT/HongKongArduinoClone
