FROM ros:humble-perception

# 1. 安裝所有必要套件
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

# 2. 安裝 noVNC
RUN git clone https://github.com/novnc/noVNC.git /opt/noVNC

# 3. 建立啟動 VNC 的腳本
RUN echo '#!/bin/bash\n\
export DISPLAY=:0\n\
Xvfb :0 -screen 0 1280x800x24 &\n\
sleep 2\n\
fluxbox &\n\
x11vnc -display :0 -forever -nopw -shared -rfbport 5900 &\n\
/opt/noVNC/utils/novnc_proxy --vnc localhost:5900 --listen 6080 &\n\
sleep 3\n\
echo "VNC Server started on http://localhost:6080/vnc.html"\n\
bash' > /usr/local/bin/start-vnc.sh \
&& chmod +x /usr/local/bin/start-vnc.sh

# 4. 環境設定
RUN echo "export DISPLAY=:0" >> /etc/bash.bashrc && \
    echo "export TURTLEBOT3_MODEL=burger" >> /etc/bash.bashrc && \
    echo "source /opt/ros/humble/setup.bash" >> /etc/bash.bashrc

# 5. 設定工作目錄
WORKDIR /workspaces
