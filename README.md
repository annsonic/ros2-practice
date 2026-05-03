# ROS2 Kilted × MediaPipe 手部辨識

本專案在 Docker 容器內執行 MediaPipe 手部辨識，訂閱主機端 ROS2 發布的 webcam 影像，辨識後將加註手部骨架的影像與手部地標資料重新發布。

```
筆電（主機）                          Docker 容器
┌─────────────────────┐              ┌──────────────────────────────┐
│ v4l2_camera_node    │─/image_raw──▶│ mediapipe_ai_service         │
│ （讀取 webcam）       │              │ （手部辨識）                   │
└──────────┬──────────┘              │  ├─ /mediapipe/annotated_image│
           │                         │  └─ /mediapipe/hand_pose      │
           │  localhost:7447         └──────────────┬───────────────┘
           └──────────────┐  ┌──────────────────────┘
                    ┌─────▼──▼──────┐
                    │  zenohd router │  （Docker service，network_mode: host）
                    └───────────────┘
        └── 所有節點透過 zenohd 互相發現與通訊（rmw_zenoh_cpp 預設） ──┘
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

### Step 2 — 建置並啟動 Docker 容器（zenohd router 先就緒）

`docker compose up` 會先啟動 `zenohd` router service，健康檢查通過後再啟動 `mediapipe_ai` 服務：

```bash
cd mediapipe_docker
docker compose build
docker compose up
```

> ⚠️ **請先執行此步驟**，待 zenohd 就緒後再啟動主機端節點（Step 3），  
> 確保主機端的 rmw_zenoh_cpp 能順利連接 localhost:7447。

### Step 3 — 主機端啟動 webcam 節點

```bash
source /opt/ros/kilted/setup.bash
# ROS2 Kilted 預設即為 rmw_zenoh_cpp，無需額外設定 RMW
# rmw_zenoh_cpp 會自動連接已在 localhost:7447 運行的 zenohd router
ros2 run v4l2_camera v4l2_camera_node
```

### Step 4 — 確認一切正常（另開終端機）

---

## 驗收

在主機端（另開終端機）執行：

```bash
source /opt/ros/kilted/setup.bash
# rmw_zenoh_cpp 為 Kilted 預設，無需額外設定

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
`docker-compose.yml` 啟動一個 **zenohd router** service（使用 Kilted 內建的 `rmw_zenohd`，  
確保與 rmw_zenoh_cpp 的協定版本完全相符），主機端與容器內的 ROS2 節點都自動連接 `localhost:7447`，  
透過 zenohd 完成節點發現與訊息傳遞，無需手動設定 RMW 或 Discovery Server。

| 設定項目 | 值 | 說明 |
|---------|-----|------|
| `network_mode` | `host` | 共用主機網路堆疊（容器與 zenohd 均使用） |
| `ipc` | `host` | 共用 IPC namespace，Zenoh 可使用共享記憶體 |
| `shm_size` | `2gb` | 共享記憶體上限 |
| `ROS_DOMAIN_ID` | `0` | 需與主機端一致（預設 0） |
| `RMW_IMPLEMENTATION` | `rmw_zenoh_cpp`（預設） | Kilted 預設值，主機端與容器端均無需額外設定 |
| zenohd | `localhost:7447` | 節點發現中介；使用 `ros2 run rmw_zenoh_cpp rmw_zenohd`，由 docker compose 自動啟動，健康檢查通過後 mediapipe_ai 才會啟動 |

> ⚠️ **啟動順序**：請先 `docker compose up`（等 zenohd 就緒），再於主機端啟動 v4l2_camera_node，  
> 確保主機端 rmw_zenoh_cpp 能連接到 zenohd router。

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
