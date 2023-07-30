# HongKongArduinoClone
 「[こーどねーむホンコン with Arduino](http://hongkongarduino.web.fc2.com)」を高速化・高機能化するプロジェクト  
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

## 対応カートリッジについて / Surpported Carts
対応状況は以下の通りです。

| Type               | Example Game      | ROM Dump | ROM Write | SRAM Dump | SRAM Write | Remarks |
| ------------------ | ----------------- | -------- | --------- | --------- | ---------- | ------- |
| LoROM　            | SHVC-MW / SHVC-FO | ○       | -         | ○        | ○         | DSPn, SuperFX, SGB BIOS, SFTurbo BIOSなどもLoROMに含まれる |
| HiROM              | SHVC-ACTJ         | ○       | -         | ○        | ○         |                          |
| ExHiROM            | SHVC-ATVJ         | ○       | -         | ○        | ○         |                          |
| SA-1               | SHVC-AKFJ         | ○       | -         | ○        | ○         | クロックモジュール回路が必要 | 
| SDD-1              | SHVC-ARFJ         | ○       | -         | ○        | ○         |                          |
| SPC7110            | SHVC-AZRJ         | ○       | -         | ○        | ○         | クロックモジュール回路が必要 | 
| CX4                | SHVC-ARXJ         | ○       | -         | -         | -          |                          |
| SpecialLoROM (3MB) | SHVC-ZDBJ         | ○       | -         | ○        | ○         |                          |
| BS-X               | SHVC-ZBSJ         | ○       | -         | ○        | ○         |                          |
| 8Mメモリーパック   | BSMC-HM-JPN       | ○       | ×        | -         | -          |                          |
| JRA PAT            | SHVC-TJCJ         | ○       | -         | ○        | ×         | SRAMの代わりにFlash      | 
| ST010 / ST011      | SHVC-E2 / SHVC-2M | ○       | -         | -         | -          | ST011は動作未確認        | 
| ST018              | SHVC-A2MJ         | ○       | -         | ○        | ○         | BIOS Dumpにも対応        | 
| MX15001            | SFメモリカセット  | ○       | ×        | ○        | ○         |                          |
| XBAND              | XBAND             | ○       | -         | ○        | ○         |                          |
| SFTurbo BaseCart   | SHVC-A9PJ         | ○       | -         | -         | -          |                          |
| SFTurbo MiniCart   | SFT-0104          | ○       | -         | ○        | ○         |                          |

+ 凡例 ○：対応　/　?：非対応　/　-：無関係

## クロックモジュールについて
HongKongArduinoの回路にクロックモジュールの回路を追加することで次のカートリッジに対応します。
 * SA-1 ROM Dump, SRAM Read, SRAM Write
 * SPC7110 Dump, SRAM Read, SRAM Write
 
[SA-1カートリッジの接続方法はこちら](https://github.com/RGBA-CRT/HongKongArduinoClone/wiki/Detect-the-SA-1-Cartridge)  

Dump from SA-1 and SPC7110 needs following Clock Modlue.  
[Connection procedure of the SA-1 Cart is here.](https://github.com/RGBA-CRT/HongKongArduinoClone/wiki/Detect-the-SA-1-Cartridge#stable-connection-method-for-sa-1-cart)  

![回路図](https://raw.githubusercontent.com/RGBA-CRT/HongKongArduinoClone/master/ss/SA1.png "回路図")   

## 動作環境 / System requirements
+ Windows Windows 2000 or later
	+ Windows 95 + IE4 でも動作しますが、一部機能が制限されます。
	+ Wineはシリアルポートの設定を行えば動作可能です。
+ Arduino UNO(ATmega 328P)
+ Arduino NANO(ATmega168)は容量不足により動作しません。
	+ クロックモジュール対応機能の削除などしてプログラムサイズを削減すれば動作可能です。
+ CH340シリアルコンバータを搭載するArduino UNO互換のボードを推奨します。
	+ aitendoの「びんぼうでいいの」など
	+ 公式のArduinoでもボーレートを下げれば動作可能です。
+ Recommend Serial Converter is CH340

## ダウンロード / Download
<https://github.com/RGBA-CRT/HongKongArduinoClone/releases>

---

## ボーレートについて / About baudrate
このソフトウェアは接続時に動的にボーレートを変更します。
ボーレートは設定から変更可能ですが、Arduinoに搭載されているシリアルコンバータや環境により動作可能な値が変わります。
つながらない, 通信エラーが多発する場合はボーレートを下げてください。

動作確認済みのボーレート：

+ Official Arduino : 500000bps 
+ Arduino with CH340 : 1000000bps
+ [詳細はこちら](https://github.com/RGBA-CRT/HongKongArduinoClone/wiki/%E9%80%9A%E4%BF%A1%E9%80%9F%E5%BA%A6%E3%83%A1%E3%83%A2)

## SFメモリカセットについて / About NP FLASH Cart
+ [接続手順](https://github.com/RGBA-CRT/HongKongArduinoClone/wiki/SF-Memory)

## 既知の問題 / Known issues
+ 問題が起きたときに読んでください
+ [here](https://github.com/RGBA-CRT/HongKongArduinoClone/wiki/Known-Issues)

## 開発環境 / Dev Environment
+ OS : Windows 7 x64
+ Arduino : Arduino UNO（ATMega328P 16MHz + CH340）... [びんぼうでいいの](https://www.aitendo.com/product/10793)
+ Language：[ActiveBasic4](https://www.activebasic.com/) + [RGBALib](https://github.com/RGBA-CRT/RGBALib)

## ライセンス / Licenses
+ たにやま氏制作部分（HongKongArduinoFast.ino）にはクリエイティブコモンズby-ncが適用されます。
[![cc](https://licensebuttons.net/l/by-nc/4.0/88x31.png)](https://creativecommons.org/licenses/by-nc/4.0/)  

+ 私が制作した部分(Windowsプログラム,　HongKongArduinoFast.ino以外のファイル)はCC0が適応されます。
[![cc0](https://licensebuttons.net/p/zero/1.0/80x15.png)](http://creativecommons.org/publicdomain/zero/1.0/)  

## 参考資料 / Referenced documents
 * <http://hongkongarduino.web.fc2.com/archive>  
 * <https://github.com/sanni/cartreader/> 
 * <http://problemkaputt.de/fullsnes.txt>  
 
---

## 履歴 / History
```
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
[2021/01/06]ver 2.0 - ST018のBIOSの吸出しに対応, ROM情報強化, CX4対応, JRA PAT対応, XBNAD対応, SufamiTurbo対応, ST01x対応, リファクタリング
```

----
Programmed by RGBA_CRT 2016-2021  
Project url: https://github.com/RGBA-CRT/HongKongArduinoClone
