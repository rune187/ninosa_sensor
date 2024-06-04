/**
  I2Cで他のボードから浮動小数点数2つを受け取り
  7セグLEDにその数値を丸めて表示する
*/
#include <SoftwareSerial.h>
#include <Wire.h>

// 赤セグのピン設定
const int redTx = 5;
const int redRx = 6;
// 青セグのピン設定
const int blueTx = 7;
const int blueRx = 8;
SoftwareSerial s7s_r(redRx, redTx);
SoftwareSerial s7s_b(blueRx, blueTx);

float receivedData1;
float receivedData2;

// セグメントに表示する文字列サイズを確保
char redString[10];
char blueString[10];

void setup() {
  Wire.begin(8);                 // I2Cバスをスレーブとしてアドレス8で開始
  Wire.onReceive(receiveEvent);  // データ受信時のイベントハンドラを設定
  Serial.begin(115200);          // シリアル通信を開始（デバッグ用）

  // セグメント設定
  s7s_r.begin(9600);
  s7s_r.write(0x76);  // 赤セグメント初期化
  s7s_r.print("-HI-");
  Serial.println("red segments init");
  delay(100);
  s7s_b.begin(9600);
  s7s_b.write(0x76);  // 青セグメント初期化
  s7s_b.print("-HI-");
  Serial.println("blue segments init");

  delay(100);
}

void loop() {
  // メインループは特に何もしない
}

void receiveEvent(int howMany) {
  if (howMany == (sizeof(receivedData1) + sizeof(receivedData2))) {
    // 受信した数値は浮動小数点数に変換されるため、float型として扱える
    // 1つ目の浮動小数点数を受信
    Wire.readBytes((char*)&receivedData1, sizeof(receivedData1));
    // 2つ目の浮動小数点数を受信
    Wire.readBytes((char*)&receivedData2, sizeof(receivedData2));

    // 受信データをシリアルモニタに表示
    Serial.print("Received Data 1: ");
    Serial.println(receivedData1);
    Serial.print("Received Data 2: ");
    Serial.println(receivedData2);

    // Serial.print("Sum: ");
    // Serial.println(receivedData1 + receivedData2);

    int dataInt1 = (int)receivedData1;
    int dataInt2 = (int)receivedData2;

    // セグメントの表示更新
    s7s_r.write(0x76);
    s7s_b.write(0x76);
    sprintf(redString, "%4d", dataInt1);
    delay(50);
    s7s_r.print(redString);
    sprintf(blueString, "%4d", dataInt2);
    s7s_b.print(blueString);
  }
}
