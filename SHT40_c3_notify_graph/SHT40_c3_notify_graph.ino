/*************************************************** 

  sparkFun Pro Micro esp32 c3
  SHT-4X series
  line notify
  ambient

 ****************************************************/

#include <Ambient.h>
#include <NTPClient.h>
#include <WiFiClientSecure.h>
#include "Adafruit_SHT4x.h"
#define LED 10

Adafruit_SHT4x sht4 = Adafruit_SHT4x();
WiFiClientSecure secureClient;
WiFiClient client;
WiFiUDP ntpUDP;
Ambient ambient;

// wifi
const char* ssid = "Arai Fruits 2.4G";
const char* pass = "00000022";

// ambient
unsigned int channelId = 77654;             //ambient channelID
const char* writeKey = "cca5290ec050af59";  //ambient writekey

// japan utc correction
const long utcOffsetInSeconds = 32400;  // 日本の場合は32400（9時間）
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// global variable
int timeCountor = 0;
int rangeFlag = 0;


//line notifyに通知を行う関数
int lineSend(String message, float temp, int SI = 0, int PSI = 0) {
  const char* host = "notify-api.line.me";
  const char* token = "7V7Px0bfjGo4M6PpcbJfiZrvrUIkO8n9OFbY6YqoAC5";  //line notify token
  const String url = "https://ambidata.io/bd/board.html?id=77081";    //line notifyに通知時のurl

  Serial.println("Try");
  if (!secureClient.connect(host, 443)) {  //LineのAPIサーバに接続
    Serial.println("Connection failed");
    return (0);
  }

  Serial.println("Connected");
  String query = "message=" + message + temp + "℃\n" + url;
  if (SI > 0) {
    query = query + "&stickerId=" + SI + "&stickerPackageId=" + PSI;
  }

  Serial.println(query);
  String request =
    String("") + "POST /api/notify HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Authorization: Bearer " + token + "\r\n" + "Content-Length: " + String(query.length()) + "\r\n" + "Content-Type: application/x-www-form-urlencoded\r\n\r\n" + query + "\r\n";
  secureClient.print(request);
  String res = secureClient.readString();
  Serial.println(res);
  secureClient.stop();
  return (1);
}


// ambientにデータを送信
void measurement(float temp, float humidity) {
  Serial.print("temp: ");
  Serial.print(temp);
  Serial.println("℃");
  Serial.print("humi: ");
  Serial.print(humidity);
  Serial.println("%");
  ambient.set(1, temp);
  ambient.set(2, humidity);
  ambient.send();
}


void setup() {
  secureClient.setInsecure();

  // init Serial
  Serial.begin(115200);
  Serial.println("Serial begin");
  pinMode(LED, OUTPUT);

  // setup SHT sensor
  Serial.println("Adafruit SHT4x test");
  if (!sht4.begin()) {
    Serial.println("Couldn't find SHT4x");
    while (1) delay(1);
  }
  Serial.println("Found SHT4x sensor");
  Serial.print("Serial number 0x");
  Serial.println(sht4.readSerial(), HEX);

  // connect wifi
  Serial.print("wifi Connecting");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(100);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  // ambient setup
  ambient.begin(channelId, writeKey, &client);

  // init NTPClient
  timeClient.begin();
  timeClient.update();

  // You can have 3 different precisions, higher precision takes longer
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

  // You can have 6 different heater settings
  // higher heat and longer times uses more power
  // and reads will take longer too!
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

  Serial.println("setup done\n");
}


void loop() {
  timeClient.update();
  int currentHour = timeClient.getHours(); // 現在の時刻（時）を取得
  int currentDayOfWeek = timeClient.getDay();  // 現在の曜日を取得（0: 日曜日、1: 月曜日、...、6: 土曜日）
  sensors_event_t humidity, temp;
  sht4.getEvent(&humidity, &temp);

  // timer reset
  if (timeCountor >= 61) {
    timeCountor = 1;
    rangeFlag = 0;
  }

  // 営業時間外は通知しない
  if (currentHour < 15 || currentHour >= 23) {
    rangeFlag = 1;
  }
  // 水曜日は休み(祝日例外設定はしていない)
  if (currentDayOfWeek == 3) {
    rangeFlag = 1;
  }

  if (temp.temperature >= 28.00 && !rangeFlag) {
    rangeFlag = 1;
    lineSend("室温が上がっています。\n温度を確認してください\n現在の温度: ", temp.temperature);
  }

  if (temp.temperature <= 15.00 && !rangeFlag) {
    rangeFlag = 1;
    lineSend("室温が下がっています。\n温度をを確認してください\n現在の温度: ", temp.temperature);
  }

  Serial.print("time: ");
  Serial.println(timeClient.getFormattedTime());
  measurement(temp.temperature, humidity.relative_humidity);
  Serial.println("");

  timeCountor += 1;
  digitalWrite(LED, HIGH);
  delay(100);
  digitalWrite(LED, LOW);
  delay(100);
  digitalWrite(LED, HIGH);
  delay(100);
  digitalWrite(LED, LOW);
  delay(4700);
}