
- i2c_scanner.cpp

程序會自動：

  - 掃描 I2C 地址 0x01 ~ 0x7E
  - 列出所有偵測到的 I2C 裝置地址（十六進制格式）
  - 顯示找到的裝置數量

預期得到兩個位置： 0x40(預設值), 0x70(all-call address，用來同步 reset 多個 PCA9685 模組) 

- servo_calibration.cpp

1. VS code 打開 Serial Monitor（115200 波特率）
2. 程式會顯示初始脈衝值 (127)
3. 逐步調整找到校準值：

  ```
  >>> 目前脈衝: 127
  輸入: +          (增加到 137)
  輸入: +          (增加到 147)
  輸入: s 1        (改為細調步長 1)
  輸入: 數字          (精細調整)
  ```

4. 觀察伺服馬達的動作，記下 中立位置 (通常 150-200) 和 極限位置 的脈衝值
5. 將這些校準值寫入 config.h 的 SERVOMIN 和 SERVOMAX

- main.cpp

1. 編輯 config.h 配置文件
2. 啟動 Micro-ROS-agent docker 容器

  ```
  docker run --rm -it   --net=host   --device=/dev/ttyUSB0:/dev/ttyUSB0   microros/micro-ros-agent:jazzy   serial --dev /dev/ttyUSB0 -b 115200
  ```

3. 監看伺服動作：

   - 伺服將自動在 -90° 到 +90° 之間擺動
   - 每 1 秒更新一次位置
   - Serial Monitor 顯示初始化進度

4. ROS2 訂閱主題 (在另一終端)：

  ```
  ros2 topic echo /micro_ros_platformio_node_publisher
  ```

  預期輸出：

  ```
  data: -90
  ---
  data: -85
  ---
  data: 0
  ---
  data: 90
  ```
