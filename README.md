# ninosa_sensor

ボード： SparkFun Pro Micro ESP32-C3
センサ： Adafruit Sensirion SHT40 Temperature & Humidity Sensor - STEMMA QT / Qwiic

qwiicケーブルで通信
adafruitのsht4Xのライブラリに付属するサンプルコードはピン番号の指定などはせずに、デフォルトで設定されるようになっているが、これを使用するにはボードの追加が手動にて必要なので以下のリンクを参考に
https://docs.sparkfun.com/SparkFun_Pro_Micro-ESP32C3/software_setup/
ボードでSparkFun Pro Micro ESP32-C3を選択してサンプルを書き込むとセンサの値を取得できる。

動作はsparkfun ESP32-C3で5秒に一回の頻度で測定してarduino nanoへI2Cでデータを渡してセグメントへ表示。
1分に1回の頻度でスプレッドシートに計測時間、温度、絶対湿度を送信している。
arduino　nanoをわざわざ使用してセグメントLEDを使用しているのはセグメントLEDにデータを送信する際に使用するSoftweareSerial, HardwearaSerialがadafruit_SHT4xのライブラリと競合してしまい、
spark funのボードから直接表示しようとするとどちらかしか使用できなかったためこのようなやり方をしている。

~~計測の期間は現状5秒に1回だが、通知が送信されてから1分は連続して5秒ごとに通知されないようになっている。温度が基準値内になっていないと1分ごとに通知を送信する。~~
現在通知機能は使用していない。

センサの標準で取得できる湿度は相対湿度で絶対湿度の方がサウナとしては体感に近い数値のため、絶対湿度へ変換している
### 相対湿度と絶対湿度について
https://www.processsensing.co.jp/blog/blog4_calculate_humidity/

