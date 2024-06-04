#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <Wire.h>
#include "Adafruit_SHT4x.h"

#define LED 10
// cicac
// #define WIFI_SSID "Arai Fruits IoT 2.4G"
// #define WIFI_PASS "00000022"
// ninosa
#define WIFI_SSID "HR02a-8ACA77"
#define WIFI_PASS "wzM0xFEesa"
#define UTC_OFFSET_IN_SECONDS 32400  // 日本の場合は32400（9時間）
#define GOOGLE_SCRIPT_KEY "AKfycbyPCiH3E8JmOUo_0VLBXm2VbzRV9LwmO3zD7GXbTPLR0cbV986TMOKjRG9QCb04EuoZ"

Adafruit_SHT4x sht4 = Adafruit_SHT4x();
WiFiClientSecure secureClient;
WiFiClient client;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", UTC_OFFSET_IN_SECONDS);

void setup() {
  // https setup
  secureClient.setInsecure();
  // I2C setup
  Wire.begin();

  Serial.begin(115200);
  Serial.println("Serial begin");
  pinMode(LED, OUTPUT);

  Serial.println("Adafruit SHT4x test");
  if (!sht4.begin()) {
    Serial.println("Couldn't find SHT4x");
    while (1) delay(1);
  }
  Serial.println("Found SHT4x sensor");
  Serial.print("Serial number 0x");
  Serial.println(sht4.readSerial(), HEX);

  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(100);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed");
    // WiFi接続失敗時の処理をここに追加
  } else {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println(WiFi.localIP());
  }

  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  switch (sht4.getPrecision()) {
    case SHT4X_HIGH_PRECISION:
      Serial.println("High precision");
      break;
    case SHT4X_MED_PRECISION:
      Serial.println("Med precision");
      break;
    case SHT4X_LOW_PRECISION:
      Serial.println("Low precision");
      break;
  }

  sht4.setHeater(SHT4X_NO_HEATER);
  switch (sht4.getHeater()) {
    case SHT4X_NO_HEATER:
      Serial.println("No heater");
      break;
    case SHT4X_HIGH_HEATER_1S:
      Serial.println("High heat for 1 second");
      break;
    case SHT4X_HIGH_HEATER_100MS:
      Serial.println("High heat for 0.1 second");
      break;
    case SHT4X_MED_HEATER_1S:
      Serial.println("Medium heat for 1 second");
      break;
    case SHT4X_MED_HEATER_100MS:
      Serial.println("Medium heat for 0.1 second");
      break;
    case SHT4X_LOW_HEATER_1S:
      Serial.println("Low heat for 1 second");
      break;
    case SHT4X_LOW_HEATER_100MS:
      Serial.println("Low heat for 0.1 second");
      break;
  }

  timeClient.begin();
  timeClient.update();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) { // WiFi接続が切れていたら再接続
    Serial.println("Wifi connection is broken, so reconnect");
    WiFi.reconnect();
    while (WiFi.status() != WL_CONNECTED) {
      digitalWrite(LED, HIGH);
      delay(100);
      digitalWrite(LED, LOW);
      delay(100);
      Serial.print(".");
    }
  }

  timeClient.update();
  sensors_event_t humidity, temp;
  sht4.getEvent(&humidity, &temp);

  Serial.print("Time: ");
  Serial.println(timeClient.getFormattedTime());
  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println("℃");
  Serial.print("Humidity: ");
  Serial.print(humidity.relative_humidity);
  Serial.println("%");
  Serial.println("");

  float sendData1 = temp.temperature;
  float sendData2 = humidity.relative_humidity;

  // I2Cでデータをボードへ送信
  delay(100);
  Wire.beginTransmission(8);
  Wire.write((byte*)&sendData1, sizeof(sendData1));
  Wire.write((byte*)&sendData2, sizeof(sendData2));
  Wire.endTransmission();  // 送信終了

  delay(100);

  // accessToGoogleSheets(timeClient.getFormattedTime(), temp.temperature, humidity.relative_humidity);

  for (int i = 0; i < 2; i++) {
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(100);
  }
  // delay(60000); // スプレッドシートへ送信する際はこちらのdelayを使用
  delay(5000);
}

void accessToGoogleSheets(String time, float temperature, float humidity) {
  HTTPClient http;
  String URL = "https://script.google.com/macros/s/" + String(GOOGLE_SCRIPT_KEY) + "/exec?";
  URL += "time=" + time + "&temperature=" + String(temperature) + "&humidity=" + String(humidity);

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
