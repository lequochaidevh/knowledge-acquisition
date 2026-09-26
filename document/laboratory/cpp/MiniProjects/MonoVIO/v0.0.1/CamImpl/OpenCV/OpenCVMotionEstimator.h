#pragma once
#include <opencv2/opencv.hpp>

#include "OpenCVVioData.h"
#include "MotionEstimator.h"

namespace VIO {
class OpenCVMotionEstimator : public MotionEstimator {
 public:
    virtual Direction Process(IFrame& iframe) override;

 private:
    cv::Mat prev;
    double  prev_cx = 0, prev_cy = 0;
    bool    has_prev_center = false;

 private:
    cv::Mat prevGray;
    bool    hasPrev = false;
};

}  // namespace VIO