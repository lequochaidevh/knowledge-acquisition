# CAMERA VIO v0.0.1

## Camera-Server (sub)
**Description:** 
Get frame at /dev/video0 and forward for multi-client to processing: VIO, Detection, ...

## VIO
**Description:**
- Config meta data with JSON.
- Build ARCH support with some software dependencies: 
1) OpenCV + [Gstreamer, V4l2].
```txt
- CPU used 80-110% (top CLI);
- Avg CPU MHz: 2500.67
```

2) Todo: Upgrade to GPU processing.

## BUILD
```sh
./build.sh
```

## RUN
```sh
# Terminal 1
cd build
sudo taskset -c 5,6 chrt -f 40 ./Camera-Server/CameraServer 5100

# Terminal 2
cd build
sudo taskset -c 3,4,5 chrt -f 30 ./VIOCameraApp
```