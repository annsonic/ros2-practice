#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  delay(2000);

  Serial.println("開始掃描 I2C...");
  int count = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("找到裝置: 0x%02X\n", addr);
      count++;
    }
  }
  if (count == 0) {
    Serial.println("沒有找到任何 I2C 裝置！請檢查接線。");
  }
}

void loop() {}
