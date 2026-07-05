#define SERVO_CHANNEL   15       // PCA9685 的 channel 15
#define SERVO_FREQ      50      // SG90 使用 50Hz
#define SERVOMIN        102     // 0° 對應的脈衝 (約 0.5ms)
#define SERVOMAX        512     // 180° 對應的脈衝 (約 2.5ms)
#define SERVO_CENTER    307     // 起始位置對應的脈衝 (中間值)
#define ANGLE_MIN       -90
#define ANGLE_MAX       90
#define ANGLE_STEP      1       // 每次移動的角度
#define CURRENT_ANGLE   -20     // SG90 起點角度
