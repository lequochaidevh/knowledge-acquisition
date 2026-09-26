#pragma once
#include "Camera/IVioData.h"
#include <opencv2/opencv.hpp>

namespace VIO {

class OpenCVFrame : public IFrame {
 public:
    OpenCVFrame(cv::Mat mat) : m_Mat(mat) {}

    int            GetWidth() const override { return m_Mat.cols; }
    int            GetHeight() const override { return m_Mat.rows; }
    const uint8_t* GetData() const override { return m_Mat.data; }

    virtual bool empty() override { return m_Mat.empty(); }
    virtual std::unique_ptr<VIO::IFrame> clone() const override {
        // 1. Deep copy the internal matrix
        cv::Mat clonedMat = m_Mat.clone();

        // 2. Wrap a new OpenCVFrame in a unique_ptr
        return std::make_unique<OpenCVFrame>(std::move(clonedMat));
    }

    cv::Mat& GetMat() { return m_Mat; }

 public:
    cv::Mat Mat() const { return m_Mat; }

 private:
    cv::Mat m_Mat;
};

class OpenCVMat : public IMat {
 public:
    virtual ~OpenCVMat() = default;

    cv::Mat Mat() const { return m_Mat; }

 private:
    cv::Mat m_Mat;
};

}  // namespace VIO