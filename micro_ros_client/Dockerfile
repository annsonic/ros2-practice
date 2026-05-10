FROM osrf/ros:kilted-desktop

ENV DEBIAN_FRONTEND=noninteractive
ENV ROS_DISTRO=kilted
ENV ROS_WS=/uros_ws

RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    python3-colcon-common-extensions \
    python3-rosdep \
    python3-venv \
    ros-kilted-rmw-cyclonedds-cpp \
    && rm -rf /var/lib/apt/lists/*

RUN rosdep init || true && rosdep update

RUN mkdir -p ${ROS_WS}/src
RUN /bin/bash -lc "source /opt/ros/${ROS_DISTRO}/setup.bash && cd ${ROS_WS}/src && git clone -b ${ROS_DISTRO} https://github.com/micro-ROS/micro_ros_setup.git"

RUN apt-get update && \
    /bin/bash -lc "source /opt/ros/${ROS_DISTRO}/setup.bash && cd ${ROS_WS} && rosdep install --from-paths src --ignore-src -y" && \
    rm -rf /var/lib/apt/lists/*
RUN /bin/bash -lc "source /opt/ros/${ROS_DISTRO}/setup.bash && cd ${ROS_WS} && colcon build"

RUN apt-get update && \
    /bin/bash -lc "source /opt/ros/${ROS_DISTRO}/setup.bash && source ${ROS_WS}/install/local_setup.bash && \
    ros2 run micro_ros_setup create_firmware_ws.sh host && \
    ros2 run micro_ros_setup configure_firmware.sh int32_publisher -t udp -i 127.0.0.1 -p 8888 && \
    python3 -m venv /opt/uros_venv && \
    /opt/uros_venv/bin/pip install --no-cache-dir numpy==1.26.4 && \
    source /opt/uros_venv/bin/activate && \
    ros2 run micro_ros_setup build_firmware.sh" && \
    rm -rf /var/lib/apt/lists/*
