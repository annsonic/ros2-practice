# micro-ROS client 與 ROS 2 通訊模擬

本專案提供一組 Docker Compose：
1. 啟動 `microros/micro-ros-agent:kilted`
2. 啟動一個 Docker 內的 micro-ROS C client（`int32_publisher`）持續發布訊息

## 檔案結構

- `./docker-compose.yml`
- `./micro_ros_client/Dockerfile`

## 前置條件

- 實體機已安裝 Docker 與 Docker Compose
- 實體機已安裝 ROS 2 Kilted（你已可正常跑 `turtlesim`）

## 啟動方式

在專案根目錄執行：

```bash
cd <你的專案根目錄>/ros2-practice
docker compose up --build
```

啟動後會有兩個服務：
- `micro_ros_agent`：監聽 UDP `8888`
- `micro_ros_client`：執行 micro-ROS C 範例 `int32_publisher`

## 在實體機 ROS 2 檢視 micro-ROS client 訊息

另開一個實體機終端機，執行：

```bash
source /opt/ros/kilted/setup.bash
export ROS_DOMAIN_ID=0

ros2 topic list
ros2 topic echo /std_msgs_msg_Int32
```

如果一切正常，`ros2 topic echo /std_msgs_msg_Int32` 會持續收到 `std_msgs/msg/Int32` 訊息。

## 常用除錯

### 1) 看容器 log

```bash
cd <你的專案根目錄>/ros2-practice
docker compose logs -f micro_ros_agent
docker compose logs -f micro_ros_client
```

### 2) 確認 Agent 有啟動

log 內應可看到 Agent 監聽 `udp4` 的相關訊息。

### 3) 關閉服務

```bash
cd <你的專案根目錄>/ros2-practice
docker compose down
```
