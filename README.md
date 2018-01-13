# HongKongArduinoClone
「こーどねーむホンコン with Arduino」用高速化ファーム＋クライアント

## 概要
このプログラムはたにやま氏が開発・公開された、"こーどねーむ「ホンコン」 with Arduino"に手を加えて
高速化・SRAMの吸出しなどの機能を加えたものです。Windows専用です。  
  
こーどねーむ「ホンコン」 with Arduino  
<http://hongkongarduino.web.fc2.com/>
	
## 開発環境
	Arduino : Aitendo製のArduino UNO互換機（ATMega328P 16MHz）
	ArduinoIDE : ver1.6.6
	HongKongArduinoとの通信速度：54179byte/sec
	開発言語：ActiveBasic ver4 + 自分用ライブラリ
	
	コンパイルにはこちらのライブラが必要です。
	https://github.com/RGBA-CRT/RGBALib


## リンク
こーどねーむ「ホンコン」 with Arduino  
<http://hongkongarduino.web.fc2.com>

たにやま氏のリポジトリ  
<https://github.com/SusumuTaniyama/HongKongArduino>

このプログラムについてのページ  
<http://rgbacrt.seesaa.net/article/435543541.html>

### 参考コード・資料
 * 	[http://hongkongarduino.web.fc2.com/archive]  
 * 	[https://github.com/sanni/cartreader/]  
 * 	[http://problemkaputt.de/fullsnes.txt]  

## ダウンロード
<https://github.com/RGBA-CRT/HongKongArduinoClone/releases>


## 特殊カートリッジについて
対応状況は以下の通りです。
 * [NG] SFメモリカセット
 * [OK] 特殊LoROM?(ダビスタ96)
 * [NG] 8Mメモリパック
 * [OK] S-DD1
 * [OK] SA-1
 * [OK] SPC7110
 * [未確認] ExHiROM
 * [未確認] CX4
 * [未確認] STシリーズ
 
SA-1とSPC7110は以下のクロックモジュール回路を追加すると吸い出せるようになります。
![回路図](https://raw.githubusercontent.com/RGBA-CRT/HongKongArduinoClone/master/SA1.png "回路図")   
![SS](https://raw.githubusercontent.com/RGBA-CRT/HongKongArduinoClone/master/SA1SS.png "SS")  　　　  　

## 履歴
	[2016/03/23]ver 0.0 - 初回リリース
	[2016/07/20]ver 0.1 - チェックサム判定機能追加
	[2017/02/17]ver 0.3 - 最適化＆バグフィックス、ファーム判定機能追加
	[2017/09/17]ver 0.4 - S-DD1に対応
	[2017/09/19]ver 0.5 - SPC7110に対応
	[2017/12/24]ver 0.6 - ダビスタ96(特殊LoROM)に対応, HiROM 256Kbit SRAMに対応