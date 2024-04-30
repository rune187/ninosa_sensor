# ninosa_sensor

ボード： SparkFun Pro Micro ESP32-C3
センサ： Adafruit Sensirion SHT40 Temperature & Humidity Sensor - STEMMA QT / Qwiic

qwiicケーブルで通信
adafruitのsht4Xのライブラリに付属するサンプルコードはピン番号の指定などはせずに、デフォルトで設定されるようになっているが、これを使用するにはボードの追加が手動にて必要なので以下のリンクを参考にすること
https://docs.sparkfun.com/SparkFun_Pro_Micro-ESP32C3/software_setup/
ボードでSparkFun Pro Micro ESP32-C3を選択してサンプルを書き込むとセンサの値を取得できる。

基本的な動作は測定してambientに送信、温度が定格範囲を上回る又は下回る際は通知をLINEで送信。
営業時間と休日（水）は稼働はしているが、通知の送信はされないように設定してある。

計測の期間は現状5秒に1回だが、通知が送信されてから1分は連続して5秒ごとに通知されないようになっている。温度が基準値内になっていないと1分ごとに通知を送信する。
