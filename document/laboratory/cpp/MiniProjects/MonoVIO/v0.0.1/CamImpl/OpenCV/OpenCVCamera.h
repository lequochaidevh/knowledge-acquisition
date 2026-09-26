
#pragma once
#include <opencv2/opencv.hpp>

#include "Camera/Camera.h"
#include "OpenCVVioData.h"

namespace VIO {
class OpenCVCamera : public Camera {
 public:
    bool Grab(IFrame& iframe) override {
        auto& frame = static_cast<OpenCVFrame&>(iframe);

        auto& mat = frame.GetMat();

        bool ok = cap.read(mat);

        return ok;
    }

    OpenCVCamera(int id, int w, int h, int fps) {
        // cap.open(id, cv::CAP_V4L2);
        // cap.set(cv::CAP_PROP_FRAME_WIDTH, w);
        // cap.set(cv::CAP_PROP_FRAME_HEIGHT, h);
        // cap.set(cv::CAP_PROP_FPS, fps);
        // cap.set(cv::CAP_PROP_FOURCC,
        //         cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));

        (void)id;
        (void)w;
        (void)h;

        std::string pipeline =
            "udpsrc port=5100 "
            "caps=\"application/"
            "x-rtp,media=video,encoding-name=JPEG,payload=26\" ! "
            "rtpjpegdepay ! jpegdec ! "
            " videorate ! video/x-raw,framerate=" +
            std::to_string(fps) +
            "/1 ! "
            "videoconvert ! appsink drop=true "
            "max-buffers=1 sync=false";

        cap.open(pipeline, cv::CAP_GSTREAMER);

        if (!cap.isOpened()) {
            std::cerr << "Failed to open GStreamer pipeline\n";
            std::exit(0);
        }
    };
    virtual ~OpenCVCamera() {
        if (cap.isOpened()) cap.release();
    };

 private:
    cv::VideoCapture cap;
};
}  // namespace VIO