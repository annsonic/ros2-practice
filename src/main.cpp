#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <micro_ros_platformio.h>

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/int32.h>

#include "config.h"

#if !defined(MICRO_ROS_TRANSPORT_ARDUINO_SERIAL)
#error This example is only avaliable for Arduino framework with serial transport.
#endif


Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

rcl_publisher_t publisher;
std_msgs__msg__Int32 msg;

rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
rcl_timer_t timer;

int current_angle = CURRENT_ANGLE;  // 目前角度
int direction = 1;              

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){error_loop();}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){}}

// Error handle loop
void error_loop() {
  while(1) {
    delay(100);
  }
}

// 將角度 (-90 ~ +90) 轉換為 PCA9685 脈衝值
uint16_t angleToPulse(int angle) {
  // angle: -90 ~ +90 對應 SERVOMIN ~ SERVOMAX
  // 0° = SERVO_CENTER
  return (uint16_t)map(angle, -90, 90, SERVOMIN, SERVOMAX);
}

void setServoAngle(int angle) {
  uint16_t pulse = angleToPulse(angle);
  pwm.setPWM(SERVO_CHANNEL, 0, pulse);
}

void timer_callback(rcl_timer_t * timer, int64_t last_call_time) {
  RCLC_UNUSED(last_call_time);
  if (timer != NULL) {
    setServoAngle(current_angle);
    msg.data = current_angle;
    RCSOFTCHECK(rcl_publish(&publisher, &msg, NULL));
    
    current_angle += direction * ANGLE_STEP;
    if (current_angle >= ANGLE_MAX) {
      current_angle = ANGLE_MAX;
      direction = -1;  // 反轉
    } else if (current_angle <= ANGLE_MIN) {
      current_angle = ANGLE_MIN;
      direction = 1;   // 正轉
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("Setting micro-ROS serial transport...");
  set_microros_serial_transports(Serial);
  delay(2000);
  Serial.println("✓ Serial transport set");

  allocator = rcl_get_default_allocator();
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
  RCCHECK(rclc_node_init_default(&node, "micro_ros_platformio_node", "", &support));
  Serial.println("✓ Node initialized");

  RCCHECK(rclc_publisher_init_default(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "micro_ros_platformio_node_publisher"));
  Serial.println("✓ Publisher initialized");

  const unsigned int timer_timeout = 1000;
  RCCHECK(rclc_timer_init_default2(
    &timer,
    &support,
    RCL_MS_TO_NS(timer_timeout),
    timer_callback,
    true
  ));
  Serial.println("✓ Timer initialized");

  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));
  Serial.println("✓ Executor initialized");

  msg.data = 0;

  Serial.println("Initializing hardware...");

  Wire.begin();
  Serial.println("✓ Wire initialized");
  
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  setServoAngle(ANGLE_MIN);
  Serial.println("✓ PWM/Servo initialized");
  delay(500);

  Serial.println("=== Setup Complete ===\n");
}

void loop() {
  delay(100);
  RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(1000)));
}
