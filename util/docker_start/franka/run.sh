#!/bin/bash
echo "--> Run set up ..."

# sudo apt install ros-dev-tools
source /opt/ros/humble/setup.sh

set -e
mkdir -p ~/franka_ros2_ws/src
cd ~/franka_ros2_ws
git clone https://github.com/frankarobotics/franka_ros2.git src
set +e

vcs import src < src/dependency.repos --recursive --skip-existing
rosdep install --from-paths src --ignore-src --rosdistro humble -y
# use the --symlinks option to reduce disk usage, and facilitate development.
colcon build --symlink-install \
 --cmake-args -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
# Adjust environment to recognize packages 
# and dependencies in your newly built ROS 2 workspace.
source install/setup.sh
# colcon test
# ros2 launch franka_fr3_moveit_config moveit.launch.py robot_ip:=dont-care use_fake_hardware:=true
# ros2 launch franka_bringup example.launch.py controller_name:=your_desired_controller
