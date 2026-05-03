FROM ros:kilted-perception

# 避免安裝過程中的互動式提問
ENV DEBIAN_FRONTEND=noninteractive

# 安裝 ROS2 kilted 套件與開發工具
RUN apt-get update && apt-get install -y \
    ros-kilted-cv-bridge \
    ros-kilted-v4l2-camera \
    ros-kilted-rviz2 \
    python3-colcon-common-extensions \
    python3-pip \
    git curl && \
    rm -rf /var/lib/apt/lists/*

# 確保每次開 shell 都會 source ROS 環境
RUN echo "source /opt/ros/kilted/setup.bash" >> /etc/bash.bashrc

WORKDIR /workspaces
