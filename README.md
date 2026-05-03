# ROS2 Kilted × MediaPipe 手部辨識

本專案在 Docker 容器內執行 MediaPipe 手部辨識，訂閱主機端 ROS2 發布的 webcam 影像，辨識後將加註手部骨架的影像與手部地標資料重新發布。

```
筆電（主機）                          Docker 容器
┌─────────────────────┐              ┌──────────────────────────────┐
│ v4l2_camera_node    │─/image_raw──▶│ mediapipe_ai_service         │
│ （讀取 webcam）       │              │ （手部辨識）                   │
└─────────────────────┘              │  ├─ /mediapipe/annotated_image│
                                     │  └─ /mediapipe/hand_pose      │
                                     └──────────────────────────────┘
           └────────── network_mode: host，DDS 自動發現 ────────────┘
```

## 前置條件

### 主機端（筆電）
- ROS2 Kilted 已安裝
- 已安裝 `v4l2_camera`：
  ```bash
  sudo apt install ros-kilted-v4l2-camera
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

### Step 2 — 主機端啟動 webcam 節點

```bash
source /opt/ros/kilted/setup.bash
# ROS2 Kilted 預設 rmw_zenoh_cpp；改用 CycloneDDS 以配合容器端設定
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
ros2 run v4l2_camera v4l2_camera_node
```

### Step 3 — 建置並啟動 Docker 容器

```bash
cd mediapipe_docker
docker compose build
docker compose up
```

---

## 驗收

在主機端（另開終端機）執行：

```bash
source /opt/ros/kilted/setup.bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

# 確認節點已上線
ros2 node list
# 預期看到：/mediapipe_ai_service

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
ROS2 底層的 DDS（Data Distribution Service）依賴 UDP multicast 做節點自動發現，  
`host` 模式讓容器內的 ROS2 節點與主機上的 ROS2 節點**完全透明地互相發現**，無需額外設定 Discovery Server 或手動對應 port。

| 設定項目 | 值 | 說明 |
|---------|-----|------|
| `network_mode` | `host` | 共用主機網路堆疊 |
| `ipc` | `host` | 共用 IPC namespace，DDS 可使用共享記憶體 |
| `shm_size` | `2gb` | 共享記憶體上限 |
| `ROS_DOMAIN_ID` | `0` | 需與主機端一致（預設 0） |
| `RMW_IMPLEMENTATION` | `rmw_cyclonedds_cpp` | 覆蓋 Kilted 預設的 rmw_zenoh_cpp；CycloneDDS 以 UDP multicast 做節點發現，`host` 模式下可直接被主機端 ROS2 發現，無需額外的 Zenoh router |

> ⚠️  **主機端也必須設定** `export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp`，否則 RMW 不一致，雙方節點無法互相發現。

---

## 專案結構

```
ros2-practice/
├── .devcontainer/              # GitHub Codespaces 驗證環境（kilted-perception）
│   ├── Dockerfile
│   └── devcontainer.json
├── mediapipe_docker/           # MediaPipe Docker 服務
│   ├── Dockerfile              # 基於 ros:kilted-perception
│   ├── docker-compose.yml      # network_mode: host
│   ├── requirements.txt        # Python 套件
│   ├── ai_service.py           # MediaPipe ROS2 節點
│   └── models/                 # （請自行放置 hand_landmarker.task）
└── README.md
```
