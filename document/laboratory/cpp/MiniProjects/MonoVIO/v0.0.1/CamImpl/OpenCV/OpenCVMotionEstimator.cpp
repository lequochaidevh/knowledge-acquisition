#include "OpenCVMotionEstimator.h"

namespace VIO {

Direction OpenCVMotionEstimator::Process(IFrame& iframe) {
    auto& frame = static_cast<OpenCVFrame&>(iframe);

    cv::Mat gray;
    cv::cvtColor(frame.GetMat(), gray, cv::COLOR_BGR2GRAY);

    if (!hasPrev) {
        prevGray = gray.clone();
        hasPrev  = true;
        return Direction::STATIC;
    }

    // ===== Convert to float =====
    cv::Mat a32, b32;
    prevGray.convertTo(a32, CV_32F);
    gray.convertTo(b32, CV_32F);

    // ===== Optional: crop center (recommended) =====
    int      w = a32.cols;
    int      h = a32.rows;
    cv::Rect roi(w * 0.2, h * 0.2, w * 0.6, h * 0.6);

    a32 = a32(roi);
    b32 = b32(roi);

    // ===== Hann window =====
    static cv::Mat hann;
    if (hann.empty() || hann.size() != a32.size()) {
        cv::createHanningWindow(hann, a32.size(), CV_32F);
    }

    cv::multiply(a32, hann, a32);
    cv::multiply(b32, hann, b32);

    // ===== Phase Correlation =====
    cv::Point2d shift = cv::phaseCorrelate(a32, b32);

    double dx = shift.x;
    double dy = shift.y;

    // ===== Deadzone =====
    if (std::abs(dx) < 2 && std::abs(dy) < 2) {
        prevGray = gray.clone();
        return Direction::STATIC;
    }

    // ===== Direction =====
    Direction dir;
    if (std::abs(dx) > std::abs(dy))
        dir = dx > 0 ? Direction::RIGHT : Direction::LEFT;
    else
        dir = dy > 0 ? Direction::DOWN : Direction::UP;

    // ===== Update state =====
    prevGray = gray.clone();

    return dir;
}

}  // namespace VIO