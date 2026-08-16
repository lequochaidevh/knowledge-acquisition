# Run ROS2 with Docker. (Support Ubuntu 20.04)

### Config docker run with X11
```sh
xhost +local:root
```

### Create `docker-compose.yml`

### Build Docker Image
```sh
docker compose up -d
```

### Run Container
```sh
docker exec -it ros2_humble_container bash
```

### Check
```sh
root@user:/#  source /opt/ros/humble/setup.bash

root@user:/#  rviz2

root@user:/#  ls /dev
```
