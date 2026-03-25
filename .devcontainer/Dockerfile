FROM ros:humble-perception

# 避免安裝過程中的互動式提問
ENV DEBIAN_FRONTEND=noninteractive

# 1. 安裝必要套件（整合並清理快取）
RUN apt-get update && apt-get install -y \
    ros-humble-turtlesim \
    ros-humble-gazebo-ros-pkgs \
    ros-humble-rviz2 \
    ros-humble-turtlebot3-gazebo \
    ros-humble-xacro \
    ros-humble-joint-state-publisher \
    ros-humble-joint-state-publisher-gui \
    xvfb fluxbox novnc websockify x11vnc \
    python3-colcon-common-extensions \
    git curl && \
    rm -rf /var/lib/apt/lists/*

# 2. 設定環境變數
ENV DISPLAY=:0
ENV TURTLEBOT3_MODEL=burger
# 確保每次開 shell 都會 source ROS 環境
RUN echo "source /opt/ros/humble/setup.bash" >> /etc/bash.bashrc

# 3. 建立啟動腳本（自動導向 index.html）
RUN echo '#!/bin/bash\n\
Xvfb :0 -screen 0 1280x800x24 &\n\
sleep 2\n\
fluxbox &\n\
x11vnc -display :0 -forever -nopw -shared -rfbport 5900 &\n\
/usr/share/novnc/utils/launch.sh --vnc localhost:5900 --listen 6080' > /usr/local/bin/start-vnc.sh \
&& chmod +x /usr/local/bin/start-vnc.sh

WORKDIR /workspaces
