#include "Config/Config.h"
#include "Camera/LoopCam.h"
#include "Camera/MotionEstimator.h"

#include "CamImpl/OpenCV/OpenCVCamera.h"
#include "CamImpl/OpenCV/OpenCVMotionEstimator.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    auto config = VIO::ConfigLoader::Load("Asset/config.json");

    std::unique_ptr<VIO::Camera> cam = std::make_unique<VIO::OpenCVCamera>(
        config.camera_id, config.width, config.height, config.fps);

    std::unique_ptr<VIO::MotionEstimator> estimator =
        std::make_unique<VIO::OpenCVMotionEstimator>();
    cv::Mat cvMat_shared_frame;

    std::unique_ptr<VIO::IFrame> shared_frame =
        std::make_unique<VIO::OpenCVFrame>(cvMat_shared_frame);

    std::mutex mtx;

    std::thread capture_thread(VIO::CaptureLoop, std::ref(cam),
                               std::ref(shared_frame), std::ref(mtx),
                               std::ref(estimator), std::ref(config));

    while (true) {
        std::unique_ptr<VIO::IFrame> frame;

        {
            std::lock_guard<std::mutex> lock(mtx);
            if (shared_frame->empty()) continue;
            frame = shared_frame->clone();
        }

        if (config.enable_ui) {
            auto& opencv_frame =
                static_cast<VIO::OpenCVFrame&>(*frame).GetMat();
            cv::imshow("Debug", opencv_frame);
            if (cv::waitKey(1) == 27) break;
        } else {
            // Backend mode
            // log or send result
        }
    }

    VIO::running = false;
    capture_thread.join();
}