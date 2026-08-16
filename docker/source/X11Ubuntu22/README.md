# Docker with x11 and toolchain for dev.

```sh
docker build -f Dockerfile -t yololabel:u22 .

export HOST_WORKSPACE="$SETUP_ENV_SCRIPT_PATH/docker/source/mount"

docker run -it --rm --net=host \
-e DISPLAY=$DISPLAY \
-e XAUTHORITY=/tmp/.Xauthority \
-v $HOST_WORKSPACE:/app/yololabel \
-v /tmp/.X11-unix:/tmp/.X11-unix \
-v /run/user/1000/gdm/Xauthority:/tmp/.Xauthority:ro \
yololabel:u22 bash
```
