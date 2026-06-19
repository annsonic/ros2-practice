# micro-ROS Wi-Fi Pub/Sub (ESP32)

透過 Wi-Fi 將 ESP32 連接至 micro-ROS Agent，實現 ROS 2 的發布/訂閱通訊。  
ESP32 訂閱 `micro_ros_name` 主題接收名字，並在 `micro_ros_response` 主題回應 `"Hello <name>!"`。

## 環境

## 環境
- 作業系統 Ubuntu 24.04
- VS Code
- ESP32 (WROOM) 開發板
- micro_ros_platformio Jazzy
- micro-ROS-Agent Jazzy docker image
- ROS2 kilted (通常是用 ROS2 Jazzy 才會一致)

## 快速開始

### 1. 設定 Wi-Fi 與 Agent 資訊

複製範本並填入你的網路資訊：

```bash
cp include/secrets.h.template include/secrets.h
```

編輯 include/secrets.h：

```
#define WIFI_SSID     "your SSID"
#define WIFI_PASSWORD "your password"
#define AGENT_IP      "192.168.x.x"
#define AGENT_PORT    "8888"
```

### 2. 編譯和燒錄
```
pio run --target upload
pio device monitor
```

預期 PlatformIO monitor 會印出 wifi 連線成功的訊息。
```
[INIT] Starting micro-ROS Wi-Fi pub/sub...
[WIFI] Connecting..
[WIFI] Connected
[WIFI] IP: 192.168.0.101  # ESP32 開發板的 IP
```

### 3. 啟動 micro-ROS Agent docker
```
docker run -it --rm --net=host -e ROS_DOMAIN_ID=8 microros/micro-ros-agent:jazzy udp4 --port 8888 -v6
```

### 4. ROS2 驗證
開啟新終端，設定 Domain ID 後，查看 topic，等待訊息：

```
source /opt/ros/<distro>/setup.bash  # <distro> 改成你安裝的 ROS2 版本
export ROS_DOMAIN_ID=8

ros2 node list          # 應出現 /wifi_pubsub_node
ros2 topic list         # 應出現 /micro_ros_name 與 /micro_ros_response
ros2 topic info /micro_ros_response   # 確認 publisher 數量是 1

ros2 topic echo /micro_ros_response std_msgs/msg/String
```

再開啟新終端，設定 Domain ID 後，發送訊息：
```
source /opt/ros/<distro>/setup.bash  # <distro> 改成你安裝的 ROS2 版本
export ROS_DOMAIN_ID=8

ros2 topic pub --once /micro_ros_name std_msgs/msg/String "{data: 'ROS2'}"
```

預期 ESP32 會回應給 /micro_ros_response：

```
data: 'Hello ROS2!'
```
