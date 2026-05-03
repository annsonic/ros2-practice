#!/usr/bin/env python3
"""
MediaPipe 手部辨識 ROS2 節點

訂閱：
  /image_raw  (sensor_msgs/Image) — v4l2_camera_node 發布的 webcam 影像

發布：
  /mediapipe/annotated_image  (sensor_msgs/Image) — 畫上手部骨架的影像
  /mediapipe/hand_pose        (std_msgs/String)    — 手部地標 JSON 字串

使用方式：
  確保 /app/models/hand_landmarker.task 已透過 volume 掛載，
  再以 docker compose up 啟動即可。
"""

import json
import os
import sys
import threading

import cv2
import mediapipe as mp
import numpy as np
import rclpy
from cv_bridge import CvBridge
from mediapipe.tasks import python as mp_python
from mediapipe.tasks.python import vision as mp_vision
from rclpy.node import Node
from sensor_msgs.msg import Image
from std_msgs.msg import String

MODEL_PATH = "/app/models/hand_landmarker.task"

# MediaPipe 手部骨架連線（用於繪圖）
HAND_CONNECTIONS = mp.solutions.hands.HAND_CONNECTIONS


class MediaPipeHandNode(Node):
    def __init__(self):
        super().__init__("mediapipe_ai_service")

        # ── 確認模型檔案存在 ──────────────────────────────────────────────
        if not os.path.isfile(MODEL_PATH):
            self.get_logger().fatal(
                f"找不到 MediaPipe 模型：{MODEL_PATH}\n"
                "請確認已將 hand_landmarker.task 放置於 mediapipe_docker/models/ 目錄，"
                "並以 docker compose up 重新啟動容器。"
            )
            sys.exit(1)

        # ── 建立 MediaPipe Hand Landmarker（LIVE_STREAM 模式）───────────
        options = mp_vision.HandLandmarkerOptions(
            base_options=mp_python.BaseOptions(model_asset_path=MODEL_PATH),
            running_mode=mp_vision.RunningMode.LIVE_STREAM,
            num_hands=2,
            min_hand_detection_confidence=0.5,
            min_hand_presence_confidence=0.5,
            min_tracking_confidence=0.5,
            result_callback=self._mediapipe_callback,
        )
        self._landmarker = mp_vision.HandLandmarker.create_from_options(options)

        # ── ROS2 發布者 / 訂閱者 ─────────────────────────────────────────
        self._pub_image = self.create_publisher(Image, "/mediapipe/annotated_image", 10)
        self._pub_pose = self.create_publisher(String, "/mediapipe/hand_pose", 10)
        self._sub = self.create_subscription(
            Image, "/image_raw", self._image_callback, 10
        )

        self._bridge = CvBridge()
        self._frame_ts = 0          # 單調遞增時間戳（毫秒），供 MediaPipe 使用
        self._lock = threading.Lock()

        self.get_logger().info("MediaPipe 手部辨識節點已啟動，等待 /image_raw 影像…")

    # ── 影像接收回呼（ROS2 executor thread）────────────────────────────
    def _image_callback(self, msg: Image):
        try:
            cv_image = self._bridge.imgmsg_to_cv2(msg, desired_encoding="rgb8")
        except Exception as e:
            self.get_logger().error(f"cv_bridge 轉換失敗：{e}")
            return

        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=cv_image)

        with self._lock:
            self._frame_ts += 1          # 確保時間戳單調遞增
            ts = self._frame_ts

        self._landmarker.detect_async(mp_image, ts)

    # ── MediaPipe 辨識結果回呼（MediaPipe 內部 thread）────────────────
    def _mediapipe_callback(
        self,
        result: mp_vision.HandLandmarkerResult,
        output_image: mp.Image,
        timestamp_ms: int,
    ):
        # 將 mp.Image 轉回 BGR OpenCV 影像以便繪圖
        frame_bgr = cv2.cvtColor(output_image.numpy_view(), cv2.COLOR_RGB2BGR)

        hands_data = []

        for hand_idx, hand_landmarks in enumerate(result.hand_landmarks):
            handedness = (
                result.handedness[hand_idx][0].display_name
                if result.handedness
                else "Unknown"
            )

            # 繪製骨架
            h, w, _ = frame_bgr.shape
            landmark_px = [
                (int(lm.x * w), int(lm.y * h)) for lm in hand_landmarks
            ]
            for start_idx, end_idx in HAND_CONNECTIONS:
                cv2.line(
                    frame_bgr,
                    landmark_px[start_idx],
                    landmark_px[end_idx],
                    (0, 255, 0),
                    2,
                )
            for px in landmark_px:
                cv2.circle(frame_bgr, px, 4, (0, 0, 255), -1)

            # 取得手腕地標（index 0）的 visibility
            wrist = hand_landmarks[0]
            hands_data.append(
                {
                    "hand": handedness,
                    "wrist_visibility": round(wrist.visibility, 3),
                    "landmarks": [
                        {"x": round(lm.x, 4), "y": round(lm.y, 4), "z": round(lm.z, 4)}
                        for lm in hand_landmarks
                    ],
                }
            )

        # ── 發布加註影像（BGR → ROS Image）──────────────────────────────
        try:
            img_msg = self._bridge.cv2_to_imgmsg(frame_bgr, encoding="bgr8")
            img_msg.header.stamp = self.get_clock().now().to_msg()
            self._pub_image.publish(img_msg)
        except Exception as e:
            self.get_logger().error(f"影像發布失敗：{e}")

        # ── 發布手部地標 JSON ────────────────────────────────────────────
        pose_msg = String()
        pose_msg.data = json.dumps(hands_data, ensure_ascii=False)
        self._pub_pose.publish(pose_msg)


def main():
    rclpy.init()
    node = MediaPipeHandNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node._landmarker.close()
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
