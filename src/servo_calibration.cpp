#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// 調整範圍設為變數
uint16_t current_pulse = 327; // 目前的脈衝值
uint16_t min_pulse = 0;       // 最小脈衝值
uint16_t max_pulse = 600;     // 最大脈衝值
uint16_t pulse_step = 10;     // 每次調整的步長

void printCurrentStatus();
void resetI2C();

void setup()
{
  Serial.begin(115200);
  delay(500);

  delay(500);

  pinMode(21, INPUT_PULLUP);
  pinMode(22, INPUT_PULLUP);
  delay(100);

  Wire.begin(21, 22, 50000);
  Wire.setTimeOut(1000);
  delay(500);

  bool found40 = false;
  for (byte addr = 0x38; addr < 0x42; addr++)
  {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
    {
      Serial.printf("找到裝置: 0x%02X\n", addr);
      if (addr == 0x40)
        found40 = true;
    }
  }
  Serial.println("掃描完成");
  if (!found40)
  {
    Serial.println("✗ 未偵測到 0x40，請檢查接線");
    while (1)
      delay(100);
  }

  Serial.println("開始初始化 PCA9685...");
  int retry = 3;
  while (retry > 0)
  {
    if (pwm.begin())
    { // 明確指定地址
      Serial.println("✓ PCA9685 初始化成功");
      break;
    }
    Serial.printf("初始化失敗，重試中... (%d)\n", retry);
    delay(500);
    retry--;
  }

  if (retry == 0)
  {
    Serial.println("✗ PCA9685 無法初始化！");
    while (1)
      delay(100);
  }

  pwm.setPWMFreq(50);

  Serial.println("\n=== 伺服馬達校準模式 ===");
  Serial.print("脈衝範圍: ");
  Serial.print(min_pulse);
  Serial.print(" - ");
  Serial.println(max_pulse);
  Serial.println("\n命令:");
  Serial.println("  + : 增加 " + String(pulse_step));
  Serial.println("  - : 減少 " + String(pulse_step));
  Serial.println("  q : 查詢目前值");
  Serial.println("  s <步長> : 設定步長");
  Serial.println("  直接輸入數字: 設定脈衝值\n");

  printCurrentStatus();
}

void printCurrentStatus()
{
  Serial.print(">>> 目前脈衝: ");
  Serial.println(current_pulse);
  pwm.setPWM(15, 0, current_pulse);
}

void loop()
{
  if (Serial.available())
  {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() == 0)
      return; // 忽略空輸入

    if (input == "+")
    {
      uint16_t new_pulse = current_pulse + pulse_step;
      if (new_pulse > max_pulse)
      {
        current_pulse = max_pulse;
      }
      else
      {
        current_pulse = new_pulse;
      }
      Serial.print("➕ 增加到: ");
      printCurrentStatus();
    }
    else if (input == "-")
    {
      if (current_pulse < pulse_step || current_pulse - pulse_step < min_pulse)
      {
        current_pulse = min_pulse;
      }
      else
      {
        current_pulse = current_pulse - pulse_step;
      }
      Serial.print("➖ 減少到: ");
      printCurrentStatus();
    }
    else if (input == "q")
    {
      Serial.print("目前脈衝值: ");
      Serial.println(current_pulse);
    }
    else if (input.startsWith("s "))
    {
      // 解析步長命令 "s 5"
      String stepStr = input.substring(2);
      uint16_t new_step = stepStr.toInt();
      if (new_step > 0 && new_step <= (max_pulse - min_pulse))
      {
        pulse_step = new_step;
        Serial.print("✓ 步長設定為: ");
        Serial.println(pulse_step);
      }
      else
      {
        Serial.println("❌ 無效步長");
      }
    }
    else
    {
      // 直接輸入數字設定脈衝值
      uint16_t pulse = input.toInt();
      if (pulse >= min_pulse && pulse <= max_pulse)
      {
        current_pulse = pulse;
        Serial.print("設定脈衝值: ");
        printCurrentStatus();
      }
      else
      {
        Serial.print("❌ 脈衝值必須在 ");
        Serial.print(min_pulse);
        Serial.print(" - ");
        Serial.print(max_pulse);
        Serial.println(" 之間");
      }
    }

    delay(200);
  }
}
