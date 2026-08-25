# Script support for auto build and run project with docker

#### version 0.0.0 Test:
```sh
export LOCAL_ASSET_PATH=/path/to/asset
# Ex: export LOCAL_ASSET_PATH=$DOCKER_WORKER/dummy_asset
# ./get_source_build_run.sh --folder=X11Ubuntu22 --branch=main
./get_source_build_run.sh --folder=ROS2 --branch=main
```

```sh
# TODO: add to the images
# $ROS_DISTRO
sudo apt update && sudo apt install -y \
  ros-humble-gazebo-ros-pkgs \
  ros-humble-xacro \
  ros-humble-joint-state-publisher-gui \
  ros-humble-robot-state-publisher \
  ros-humble-teleop-twist-keyboard \
  ros-humble-twist-mux \
  ros-humble-controller-manager \
  ros-humble-ros2-control \
  ros-humble-ros2-controllers \
  ros-humble-gazebo-ros2-control \
  git vim

# sudo apt install ros-humble-gazebo-ros-pkgs -y

# franka
sudo apt-get update
sudo apt-get install -y ros-humble-moveit-core
# franka
cd /path/to/ros2_ws

git clone https://github.com/joshnewans/articubot_one.git
source /opt/ros/humble/setup.bash
colcon build --symlink-install
source install/setup.bash

sudo ln -s /opt/ros/humble/lib/controller_manager/spawner /opt/ros/humble/lib/controller_manager/spawner.py

ros2 launch articubot_one launch_sim.launch.py world:=./src/articubot_one/worlds/obstacles.world

# export LIBGL_ALWAYS_SOFTWARE=1
# export QSG_RENDER_LOOP=basic
# ros2 launch articubot_one launch_sim.launch.py
# killall -9 gzserver gzclient
# ros2 daemon stop
# ros2 daemon start
# pkill -9 -f gazebo
# pkill -9 -f gzserver
# pkill -9 -f gzclient
# pkill -9 -f ros2
# pkill -9 -f spawner
# rm -rf /tmp/.gazebo/
# rm -rf ~/.gazebo/
```

```sh
# Terminal 2
ros2 topic list
ros2 run rviz2 rviz2 -d articubot_one/config/view_bot.rviz
# Terminal 3
source /opt/ros/humble/setup.bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard
```