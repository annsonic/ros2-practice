FROM ros:kilted-perception

# 避免安裝過程中的互動式提問
ENV DEBIAN_FRONTEND=noninteractive

# 安裝系統套件：cv_bridge 使用 apt 版本，避免與 ROS2 ABI 衝突
RUN apt-get update && apt-get install -y \
    python3-pip \
    ros-kilted-cv-bridge && \
    rm -rf /var/lib/apt/lists/*

# 安裝 Python 套件
WORKDIR /app
COPY requirements.txt .
RUN pip3 install --no-cache-dir --break-system-packages -r requirements.txt

# 複製節點程式
COPY ai_service.py .

# source ROS2 環境並啟動節點
# models/ 目錄由 docker-compose volumes 掛載，不在此 COPY
CMD ["/bin/bash", "-c", "source /opt/ros/kilted/setup.bash && python3 /app/ai_service.py"]
