/*-------------------------------------------------------------------------------------
					HongKongArduino 高速版 [2016/7/20] 
	
	オリジナルの作者 : (c) 2014 たにやま  [http://hongkongarduino.web.fc2.com/]
	　高速版作者者　 : RGBA_CRT 2016/7/20 [rgba3crt1p@gmail.com]
-------------------------------------------------------------------------------------*/

◇概要
	このプログラムはたにやま氏が開発・公開された、"こーどねーむ「ホンコン」 with Arduino"に手を加えて
	高速化・SRAMの吸出しなどの機能を加えたものです。

	[本家]こーどねーむ「ホンコン」 with Arduino
	http://hongkongarduino.web.fc2.com/
	
	高速版のページ
	http://rgbacrt.seesaa.net/article/435543541.html
	
	開発ページ
	https://github.com/RGBA-CRT/HongKongArduinoClone


◇注意
	本プログラムを使用していかなる破損・災害が発生したとしても作者は一切責任を取りません。
	特にカートリッジ内部のセーブデータ領域のデータは消えやすいので、そのことを踏まえた上でご使用ください。
	すべてのカートリッジにおいて正確なROM・SRAMの吸出しを保証はしません。
	特にSRAMの書き込み機能は不安定なので、消えてほしくないデータがカートリッジにある場合には使用しないでください。
	また、吸い出したROMイメージをインターネット上に公開することは犯罪です。


◇セット内容
	HongKongArduinoClone.exe		->	Windows用吸出しプログラムです。
	HongKongArduinoCloneSrc.zip	->	上記ファイルのソースコードです。
	
	firmware\ArduinoHexWriter.exe	->	Arduinoへファームウェアを書き込むためのバッチファイル作成プログラムです。
	firmware\ArduinoHexWriterSrc.zip	->	上記ファイルのソースコードです。
	firmware\HongKongArduinoFast.ino	->	Arduinoのスケッチです。
	firmware\HongKongArduinoFast.ino.standard.hex	->	コンパイル済みArduino用バイナリです。これをArduinoへ書き込んでください。


◇特殊カートリッジについて
	対応状況は以下の通りです。
	 * [NG] SFメモリカセット
	 * [NG] 特殊LoROM?(ダビスタ96)
	 * [NG] 8Mメモリパック
	 * [未テスト] ExHiROM(テイルズ)
	 * [OK] S-DD1
	 * [OK] SA-1
	SA-1はSA1.pngの回路を追加することで吸出し可能になります。
	（無理のある配線をしているのでArduinoのバージョンによっては動作しない可能性があります。）


◇使い方
	先に回路が完成し、公式のクライアント（WinHongKong）での正常な吸出しができることが前提です。

	1.Arduinoにファームウェアを書き込み
		HongkongArduinoFastフォルダに	HongKongArduinoFast.hexというファイルがあるので、Arduinoを接続して書き込んでください。
		※初回のみ行ってください。
		
		ArduinoHexWriter.exeを使うと比較的簡単に書き込めます。
			ArduinoIDEフォルダは、arduino.exeがあるフォルダを指定してください。確認ボタンを押して「正常」と表示されることを確認してください。
			ArduinoHexWriterを起動し、一番下のテキストボックスにhexファイルをドロップするとパスが入力されます。
			「書き込み」ボタンを押すとArduinoにHEXファイルを書き込みます。
			コマンドプロンプトが表示され、「avrdude.exe done. Thank you.」と表示されたら書き込みに成功しています。
			何かキーを押して終了してください。
		
		なお、このHEXファイルは最適化オプション-O3(処理速度優先)でコンパイルされています。
		デフォルトのArduinoIDEの設定では最適化オプションが-Os(サイズの小ささ優先)になっています。
		
	2.クライアントの起動
		HongKongArduinoClone.exeを起動してください。
		「Serial Port」にArduinoのポートを指定して[connect]ボタンを押してください。
		右のInfomation欄に「Connection successful!」が表示され、ゲームタイトルが表示されていれば接続成功です。
		もし、ファームウェアを変更して通信レートを変更した場合はダイアログ右の[baudrate]を変更してください。
		
	3.クライアントの操作
		3-1 カートリッジを接続し、GetCartInfoボタンを押してゲーム情報を取得してください。これをしないとLoROM/HiROMの判定がされません。
		3-2 DumpROMで吸出しを行います。
		3-3 DumpSRAMでSRAMの吸出しを行います。うまく吸い出せなかった場合はMAD-1のチェックを入れて吸出しをしてみてください。
		
	4.終了
		[Close]ボタンを押すとArduinoとの接続を切断します。
		

◇開発環境
	動作可能環境の目安にしてください。
	OS 	: Win7 SP1　32bit
	CPU 	: Core i3 330M
	Arduino : Aitendo製のArduino UNO互換機（ATMega328P 16MHz 5.0v）
	ArduinoIDE : ver1.6.6
	HongKongArduinoとの通信速度：54179byte/sec
	開発言語：ActiveBasic ver4 + 自分用ライブラリ

◇Q&A
	・吸出し中に[0]Warning!　と出るが、これは何か？
		送信されてきたデータのサイズが不正の時に表示されます。
		
	・「Over retry count.」と表示されて吸出しが停止した
		上記のWarningが20回ほど発生した場合に吸出しが停止します。
		HongKongArduinoClone.iniの中のMaxContinueの右辺の値を増やすことでWarning時の再試行回数を増やせます。
		
	・Serial Portの欄にErrと表示される
		COMポートのリストアップに必要なメモリが不足している可能性があります。
		HongKongArduinoClone.iniの中のQDDSizeの右辺を書き換えると改善する可能性があります。
		なお、QDDSizeの値が1023以下の場合デフォルトの値に戻されますので注意してください。
		
	・ゲームタイトルなどが正常に表示されない
	・吸い出したデータのCRCが合わない
		カセットの接触が悪い可能性があります。綿棒にサビ取り剤などをつけて端子を掃除してください。
		また、いったん接続を切り、USBケーブルを抜いたのち、もう一度再接続すると改善する場合があります。
		それでもダメな場合はあきらめてください。
		
	・ゲームが正常に吸い出せない
		非対応のカートリッジの場合があります。
		
	・セーブデータ（SRAM）が正常に吸い出せない
		MAD1のオプションを試してみてください。
		SRAMの吸出しの動作確認済みのカートリッジは以下の通りです。
		・ノーマルLoROMカートリッジ
		・MAD1搭載HiROMカートリッジ
		・SA1搭載カートリッジ。
		SA1については書き込みはできません。
		またMAD1を搭載していないのにMAD1にチェックをつけないとSRAMが吸い出せないカートリッジもありました。
		
	・バグがある/スペルが間違っている/要望がある
		作者であるRGBA_CRTへ連絡[rgba3crt1p@gmail.com]してください。ただし、直せるとは限りません。
		なお、高速版に関する連絡はオリジナルの作者である たにやま氏には迷惑になるので止めてください。
		
	・ソースコードのライセンスは？
		Arduino用のファームウェアはたにやま氏の制作物であるので、原版と同じく「クリエイティブ・コモンズ 表示 - 非営利 4.0 国際 ライセンス」が適用されます。
		HongKongArduinoCloneはたにやま氏のWinHongKongArduinoのソースコードや資料を参考に作ったものなので同じく
		「クリエイティブ・コモンズ 表示 - 非営利 4.0 国際 ライセンス」を適用します。
		
	・通信速度を変更したい
		HongKongArduinoFast.ino の setup()内のSerial.begin(500000);が通信速度です。
		お好みで変更してください。
		なお、ArduinoIDEのデフォルトの最適化オプションは-Osなので-O3に設定変更することをお勧めします。
		
	・HongKongArduinoClone.exeのソースコードについて
		ActiveBasic ver4で開発しています。[http://www.activebasic.com/ab4_download.html]
		文法はBASICの皮をかぶったC言語というような感じなのですぐわかると思います。
		
		
	
◇謝辞
	このプログラムを作るにあたって、たにやま氏のWinHongKongArduinoのソースコード、
	そして「回路の動作原理とプログラム作成のヒント」やパラレル版のホンコンのソースコードを参考にして作りました。
	回路の部分も非常に勉強になりました。ここに感謝の意を表します。
	
	
◇連絡先
	E-mail : rgba3crt1p@gmail.com
	

◇ 履歴
	[2016/03/23]ver 0.0 - 初回リリース
	[2016/07/20]ver 0.1 - チェックサム判定機能追加
	[2017/02/17]ver 0.3 - 最適化＆バグフィックス、ファーム判定機能追加
	[2017/09/17]ver 0.4 - S-DD1に対応
	[2017/09/19]ver 0.5 - SPC7110に対応
	[2017/12/24]ver 0.6 - ダビスタ96(特殊LoROM)に対応