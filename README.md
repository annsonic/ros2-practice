# ROS2 Kilted × MediaPipe 手部辨識

本專案在 Docker 容器內執行 MediaPipe 手部辨識，訂閱主機端 ROS2 發布的 webcam 影像，辨識後將加註手部骨架的影像與手部地標資料重新發布。

```
筆電（主機）                          Docker 容器
┌─────────────────────┐              ┌──────────────────────────────┐
│ v4l2_camera_node    │─/image_raw──▶│ mediapipe_ai_service         │
│ （讀取 webcam）       │              │ （手部辨識）                   │
└──────────┬──────────┘              │  ├─ /mediapipe/annotated_image│
           │                         │  └─ /mediapipe/hand_pose      │
           └────── network_mode: host（DDS multicast 自動互相發現）──┘
      └── 兩端均使用 RMW_IMPLEMENTATION=rmw_cyclonedds_cpp ──┘
```

## 前置條件

### 主機端（筆電）
- ROS2 Kilted 已安裝
- 已安裝 `v4l2_camera` 與 CycloneDDS：
  ```bash
  sudo apt install ros-kilted-v4l2-camera ros-kilted-rmw-cyclonedds-cpp
  ```

### Docker 端
- 已安裝 Docker Engine 與 Docker Compose Plugin
- 已下載 MediaPipe Hand Landmarker 模型（`.task` 檔）

---

## 啟動步驟

### Step 1 — 放置模型檔

將 `hand_landmarker.task` 放到 `mediapipe_docker/models/` 目錄下：

```bash
mkdir -p mediapipe_docker/models
cp /path/to/hand_landmarker.task mediapipe_docker/models/
```

> 下載連結（若尚未下載）：
> ```bash
> wget -q -O mediapipe_docker/models/hand_landmarker.task \
>   https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/latest/hand_landmarker.task
> ```

### Step 2 — 建置並啟動 Docker 容器

```bash
cd mediapipe_docker
docker compose build
docker compose up
```

### Step 3 — 主機端啟動 webcam 節點

```bash
source /opt/ros/kilted/setup.bash
# 必須與容器端使用相同的 RMW，才能透過 DDS multicast 互相發現
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
ros2 run v4l2_camera v4l2_camera_node
```

### Step 4 — 確認一切正常（另開終端機）

---

## 驗收

在主機端（另開終端機）執行：

```bash
source /opt/ros/kilted/setup.bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

# 確認節點已上線（應看到 /mediapipe_ai_service）
ros2 node list

# 確認主題已發布
ros2 topic list
# 預期看到：
#   /image_raw
#   /mediapipe/annotated_image
#   /mediapipe/hand_pose

# 查看手部地標資料（JSON 格式）
ros2 topic echo /mediapipe/hand_pose

# 查看加註影像的頻率
ros2 topic hz /mediapipe/annotated_image
```

---

## 網路設定說明

容器使用 `network_mode: host`，直接共用主機的網路介面與 loopback。  
兩端均使用 `RMW_IMPLEMENTATION=rmw_cyclonedds_cpp`，CycloneDDS 透過 DDS multicast 自動完成  
節點發現與訊息傳遞，**不需要獨立的 zenohd router 程序**，設定最簡單也最可靠。

| 設定項目 | 值 | 說明 |
|---------|-----|------|
| `network_mode` | `host` | 共用主機網路堆疊，DDS multicast 可到達主機與容器 |
| `ipc` | `host` | 共用 IPC namespace，DDS 可使用共享記憶體 |
| `shm_size` | `2gb` | 共享記憶體上限 |
| `ROS_DOMAIN_ID` | `0` | 需與主機端一致（預設 0） |
| `RMW_IMPLEMENTATION` | `rmw_cyclonedds_cpp` | **容器與主機端均需設定**，確保使用相同 RMW |

---

## 專案結構

```
ros2-practice/
├── .devcontainer/              # GitHub Codespaces 驗證環境（kilted-perception）
│   ├── Dockerfile
│   └── devcontainer.json
├── mediapipe_docker/           # MediaPipe Docker 服務
│   ├── Dockerfile              # 基於 ros:kilted-perception，含 rmw-cyclonedds-cpp
│   ├── docker-compose.yml      # network_mode: host + RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
│   ├── requirements.txt        # Python 套件
│   ├── ai_service.py           # MediaPipe ROS2 節點
│   └── models/                 # （請自行放置 hand_landmarker.task）
└── README.md
```
