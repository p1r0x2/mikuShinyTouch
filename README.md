# mikuShinyTouch

## 概要

電子ピアノ(キーボード)の打鍵に合わせて弾いた箇所の鍵盤を光らせるためのソフトウェアです。
タッチの強弱やダンパーペダルの踏み替えを光の強弱や減衰に反映します。
初音ミクカラー(エメラルドグリーンとピンク)で光るように設定してありますが、色は自由に設定できます。

制御基板、LEDテープと組み合わせて使います。
USB-MIDIの信号を出力できる電子ピアノであれば、いくつかの設定値の変更で対応できるはずです。

追って改訂しますがとりあえず置いておきます。

## 使用例

https://www.nicovideo.jp/watch/sm36324927

## 全体構成

```
                  +----------------------+
+-----------+     |                      |
| 12V Power +---->+ LED tape (NeoPixel)  +<--(Signal + GND)-----------------+
+-----------+     |                      |                                  |
                  +----------------------+                                  |
                                                             +--------------+--------------+
                                                             |   3.3V to 5V Level shifter  |
                                                             +--------------+--------------+
                                                                            ^
                                                                            |
                                                             +--------------+--------------+
                                                             |                             |     +----------+
                                                             |   mikuShinyTouch (Teensy)   +<----+ 5V Power |
                                                             |                             |     +----------+
                                                             +--------------+--------------+
                  +----------------------+                                  ^
                  |                      |                                  |
                  |    Piano/Keyboard    +----(USB cable)-------------------+
                  |                      |
                  +----------------------+
```

## 使用手順

1. 事前準備
    1. 部品調達
    2. 基板作成・LEDテープ接続
    3. Teensyduino環境を導入
2. mikuShinyTouchをTeensyに書き込み
3. 必要に応じて設定値をピアノに合わせて調整
4. 弾くと光って楽しい

## 事前準備

### 部品調達例

* [Teensy 4.0](https://www.sengoku.co.jp/mod/sgk_cart/detail.php?code=EEHD-5JBL)

* [ＵＳＢコネクタＤＩＰ化キット　（Ａメス）](http://akizukidenshi.com/catalog/g/gK-07429/)

* [WS2815 12V NeoPixel RGB テープLED 60LED/m](https://www.akiba-led.jp/product/1782)

* [アルミコーナーフレーム角形 16×16×1m](https://www.akiba-led.jp/product/679)

* [ACアダプター 12V 1.5A](https://www.akiba-led.jp/product/64)

* [赤黒DCジャックケーブル 1m](https://www.akiba-led.jp/product/1267)

* [８ビット双方向ロジックレベル変換モジュール](http://akizukidenshi.com/catalog/g/gM-04522/)
    * 5V側にダイオードを設置
    * WS2815の通信速度は最大800Kbps

* パスコン
    * 12V電源ラインに1000uF
    * 5V電源ラインに220uF

* ユニバーサル基板

(使用例の構成ですが、5V電源ラインのノイズに弱い問題があるので改訂予定)

### 基板作成・LEDテープ接続

* USB AメスコネクタのVCC, D-, D+, GNDをTeensyの5V, D-, D+, GND端子へ接続
* Teensyの17番ピンを3.3Vから5Vにレベル変換してLEDテープのDIへ接続
* TeensyのGND端子をLEDテープのGNDへ接続
* LEDテープの電源線にDCジャックケーブルを接続して熱収縮チューブなどで接合部を絶縁
* パスコンを設置

## ピアノに合わせて調整する設定値 (抜粋)

### KEYPOS

各鍵盤の位置。
LEDを光らせる位置の計算に使用する。

左端の鍵盤の中心を0mmとして各鍵盤の中心の相対位置をミリ単位で記述する。
デフォルト値は使用例の電子ピアノ(Roland DP603)の鍵盤位置をおおまかに測定・計算したもの。

### KEYNUM_LEDSTART, KEYNUM_LEDEND

LEDテープの左端と右端に対応する鍵盤の番号。
1mのLEDテープとアルミフレームを使う場合、88鍵のピアノでは鍵盤全体をカバーできないので、どこからどこまでの鍵盤をカバーして光らせるか指定する。

各鍵盤に割り振られる番号は、左端の鍵盤を0番として右に行くに連れて番号が1ずつ増えるように計算する。

## ライセンス

MIT Licenseとします。
LICENSEファイルを参照してください。
