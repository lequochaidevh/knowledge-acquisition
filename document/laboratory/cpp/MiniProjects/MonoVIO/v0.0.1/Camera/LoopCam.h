#pragma once

#include <thread>
#include <atomic>
#include <chrono>
#include <fstream>
#include <stdexcept>
#include <mutex>
#include <iostream>

#include "Config/Config.h"
#include "Camera/Camera.h"
#include "CamImpl/CamImpl.h"
#include "MotionEstimator.h"

namespace VIO {

std::atomic<bool> running = true;

void CaptureLoop(std::unique_ptr<Camera>& cam,
                 std::unique_ptr<IFrame>& shared_frame, std::mutex& mtx,
                 std::unique_ptr<MotionEstimator>& estimator,
                 AppConfig&                        config) {
    int  target_fps = config.fps;
    auto frame_time = std::chrono::milliseconds(1000 / target_fps);

    while (running) {
        auto start = std::chrono::steady_clock::now();

        std::unique_ptr<IFrame> frame = CreateIFrame(config.VIO_type);
        if (!cam->Grab(*frame.get())) {
            std::cout << "Grab Empty\n";
            continue;
        }
        if (frame.get()->empty()) {
            std::cout << "Frame Empty\n";
            continue;
        }

        // if (frame_count++ % config.skip_frame != 0) {
        //     continue;  // skip frame
        // }

        auto dir = estimator->Process(*frame.get());

        switch (dir) {
            case Direction::LEFT:
                std::cout << "LEFT\n";
                break;
            case Direction::RIGHT:
                std::cout << "RIGHT\n";
                break;
            case Direction::UP:
                std::cout << "UP\n";
                break;
            case Direction::DOWN:
                std::cout << "DOWN\n";
                break;
            default:
                break;
        }

        auto end     = std::chrono::steady_clock::now();
        auto elapsed = end - start;

        if (config.enable_ui) {
            std::lock_guard<std::mutex> lock(mtx);
            shared_frame = std::move(frame);
        }
        if (elapsed < frame_time) {
            std::this_thread::sleep_for(frame_time - elapsed);
        }
    }
}
}  // namespace VIO