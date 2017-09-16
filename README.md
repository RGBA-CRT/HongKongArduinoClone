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
 * [NG] 特殊LoROM?(ダビスタ96)
 * [NG] 8Mメモリパック
 * [未テスト] ExHiROM(テイルズ)
 * [OK] S-DD1
 * [OK] SA-1
 
SA-1は以下の回路を追加すると吸い出せるようになります。
![回路図](https://raw.githubusercontent.com/RGBA-CRT/HongKongArduinoClone/master/SA1.png "回路図")   
![SS](https://raw.githubusercontent.com/RGBA-CRT/HongKongArduinoClone/master/SA1SS.png "SS")  　　　  　

