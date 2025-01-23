// M5toughで測定した温度を表示しつつスプレッドシートに送信
// M5toughではピンは25番じゃないと他機能との競合で温度が取得できなかった
#include <DallasTemperature.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <M5Unified.h>
#include <OneWire.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <time.h>
#include <SD.h>

// DS18B20のデータピンを接続したArduinoのデジタルピン番号
#define ONE_WIRE_BUS 25

// WiFi設定
// cicac
const char* ssid = "New Terauchi-2G";  // Wi-FiのSSID
const char* password = "00000022";     // Wi-Fiのパスワード
// nonosa
// const char* ssid = "HR02a-8ACA77";    // Wi-FiのSSID
// const char* password = "wzM0xFEesa";  // Wi-Fiのパスワード

// NTPサーバー
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 9 * 3600;  // 日本はGMT+9
const int daylightOffset_sec = 0;     // サマータイムは使用しない
unsigned long lastExecutionTime = 0;  // 最後に処理を実行した時刻を記録

// OneWireとDallasTemperatureライブラリのオブジェクトを作成
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// WiFi Secureのインスタンスを作成
WiFiClientSecure secureClient;

void setup() {
  // https setup
  secureClient.setInsecure();
  auto cfg = M5.config();
  cfg.external_display.unit_lcd = true;  // LCDディスプレイを有効化
  M5.begin(cfg);

  // 初期メッセージをLCDに表示
  M5.Display.clearDisplay();  // 画面をクリア
  M5.Display.setTextSize(2);  // テキストサイズを設定
  M5.Display.println("Connecting to Wi-Fi...");

  // Wi-Fiに接続
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    M5.Display.print(".");
  }
  Serial.println("\nWiFi connected!");
  M5.Display.println("\nWiFi connected!");

  // NTPサーバーの設定
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("NTP server configured.");

  M5.Lcd.setTextFont(&fonts::efontJA_16);  // 日本語表示を有効化
  Serial.begin(9600);                      // シリアル通信を開始
  sensors.begin();                         // DS18B20センサーを初期化
  Serial.println("DS18B20 Temperature Sensor Initialization Complete");
  M5.Display.clearDisplay();  // 画面をクリア
}

void loop() {
  // WiFiが切れていたら再接続
  if (WiFi.status() != WL_CONNECTED) {  // WiFi接続が切れていたら再接続
    Serial.println("Wifi connection is broken, so reconnect");
    M5.Display.setCursor(10, 10);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.println("WiFi disconnect");
    WiFi.reconnect();
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      M5.Display.print(".");
    }
  }

  uint16_t tempColor = TFT_GREEN;                  // 温度表示のカラー（デフォルトは緑）
  M5.update();                                     // タッチ情報の更新
  sensors.requestTemperatures();                   // 温度をリクエスト
  float temperature = sensors.getTempCByIndex(0);  // 温度を取得（摂氏）

  if (temperature != DEVICE_DISCONNECTED_C) {
    // 温度によって文字色を変更
    if (temperature >= 82) { // 本番環境用
      tempColor = TFT_RED;
    } else if (temperature >= 75) {
      tempColor = TFT_ORANGE;
    } else if (temperature >= 70) {
      tempColor = TFT_YELLOW;
    }

    if (temperature >= 31) {  // テスト用
      tempColor = TFT_RED;
    } else if (temperature >= 29) {
      tempColor = TFT_ORANGE;
    } else if (temperature >= 27) {
      tempColor = TFT_YELLOW;
    }
  } else {
    Serial.println("Error: Temperature Sensor Not Found");
    tempColor = TFT_WHITE;  // センサーが見つからない場合は白色
    temperature = 0;        // 表示する温度を0にする
  }

  // 現在時刻を取得
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    Serial.println("Failed to obtain time");
    M5.Display.setCursor(10, 30);
    M5.Display.println("Failed to get time");
    delay(1000);
    return;
  }
  // 時刻をフォーマット
  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeInfo);

  // 時間の表示
  M5.Display.setCursor(10, 10);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.println(timeStr);
  // 日本語表示
  M5.Display.setCursor(10, 30);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);  // 通常のテキストは白
  M5.Display.println("只今の温度");
  // 温度の表示
  M5.Display.setCursor(10, 70);
  M5.Display.setTextColor(tempColor, TFT_BLACK);  // 温度のテキストは色を変更
  M5.Display.setTextSize(4);
  M5.Display.print(String(temperature) + "°C");

  unsigned long currentMillis = millis(); // 現在の時刻をms単位で取得

  // 前回のデータ送信から1分以上経過していたら再度データを送信
  if (currentMillis - lastExecutionTime >= 60000) {
    lastExecutionTime = currentMillis;  // 最終実行時刻を更新

    // スプレッドシートにgetリクエストで時刻と温度のデータを送信
    accessToGoogleSheets(timeStr, temperature);
  }

  M5.delay(500);
}

// getリクエストする際の文字列をURLエンコード
String URLEncode(const String& value) {
  String encoded = "";
  const char* hex = "0123456789ABCDEF";
  for (int i = 0; i < value.length(); i++) {
    char c = value.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      encoded += '%';
      encoded += hex[(c >> 4) & 0xF];
      encoded += hex[c & 0xF];
    }
  }
  return encoded;
}

// スプレッドシートにデータを送信
void accessToGoogleSheets(String time, float temperature) {
  String time_last5 = time.substring(time.length() - 8, time.length() - 3);  // 日時のデータから末尾8文字を抽出（hh:mm:ss）
  HTTPClient http;
  String URL = "https://script.google.com/macros/s/AKfycbweSG6jc_QcB3NARs-46RPJvjNx-1tlUfDxU6TzRrcpmcEkw875grUebAS3q68EFiBl/exec?";
  URL += "time=" + URLEncode(time_last5) + "&temperature=" + URLEncode(String(temperature));

  http.begin(URL);
  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.println(httpCode);  // この中のif節に入ってきていないことからHTTP_CODE_OKにはなっていない説がある
    if (httpCode == HTTP_CODE_OK) {
      Serial.println(httpCode);
    }
  } else {
    Serial.print("[HTTP] GET failed, error: ");
    Serial.println(http.errorToString(httpCode).c_str());
  }

  http.end();  // HTTPClientの解放
}