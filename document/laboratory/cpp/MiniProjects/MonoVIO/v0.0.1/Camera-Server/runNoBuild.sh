#!/bin/bash
gst-launch-1.0 -v filesrc location=VID_MOVE_1.mp4 do-timestamp=true ! decodebin3 ! videoconvert ! videoscale ! videorate ! video/x-raw,format=I420,width=640,height=480,framerate=30/1 ! jpegenc quality=85 ! jpegparse ! rtpjpegpay ! udpsink host=127.0.0.1 port=5100 sync=true
