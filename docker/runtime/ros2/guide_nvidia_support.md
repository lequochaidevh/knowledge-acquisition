# Docker GPU runtime with NVIDIA support (non test)

```sh
# GPG key to NVIDIA container
curl -fsSL https://github.io | sudo gpg --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg
curl -s -L https://github.io | \
  sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' | \
  sudo tee /etc/apt/sources.list.d/nvidia-container-toolkit.list

# Update and install toolkit
sudo apt-get update
sudo apt-get install -y nvidia-container-toolkit

# reconfig Docker for runtime from NVIDIA
sudo nvidia-container-toolkit xorg-configure
sudo systemctl restart docker
```

#### docker-compose
```yml
version: '3.8'

services:
  ros2_humble:
    image: osrf/ros:humble-desktop-full
    container_name: ros2_humble_container
    network_mode: host
    ipc: host
    pid: host
    privileged: true
    environment:
      - DISPLAY=${DISPLAY}
      - QT_X11_NO_MITSHM=1
      - NVIDIA_VISIBLE_DEVICES=all
      - NVIDIA_DRIVER_CAPABILITIES=all
    volumes:
      - /tmp/.X11-unix:/tmp/.X11-unix:rw
      - /dev:/dev:rw
      - ./workspace:/home/ros/workspace:rw
    deploy:
      resources:
        reservations:
          devices:
            - driver: nvidia
              count: all
              capabilities: [gpu]
    stdin_open: true
    tty: true
    restart: unless-stopped
    
version: '3.8'

services:
  ros2_humble:
    image: osrf/ros:humble-desktop-full
    container_name: ros2_humble_container
    network_mode: host
    ipc: host
    pid: host
    privileged: true
    environment:
      - DISPLAY=${DISPLAY}
      - QT_X11_NO_MITSHM=1
      - NVIDIA_VISIBLE_DEVICES=all
      - NVIDIA_DRIVER_CAPABILITIES=all
    volumes:
      - /tmp/.X11-unix:/tmp/.X11-unix:rw
      - /dev:/dev:rw
      - ./workspace:/home/ros/workspace:rw
    deploy:
      resources:
        reservations:
          devices:
            - driver: nvidia
              count: all
              capabilities: [gpu]
    stdin_open: true
    tty: true
    restart: unless-stopped

