#pragma once
#include "OpenCVVioData.h"

namespace VIO {

enum class VIOCamImplType {
    OpenCV,
};

// class CamImpl {
//  private:
//     std::unique_ptr<VIO::IFrame> m_Frame;

//  public:
//     CamImpl(/* args */);
//     ~CamImpl();
// };

// CamImpl::CamImpl(/* args */) {}

// CamImpl::~CamImpl() {}

std::unique_ptr<IFrame> CreateIFrame(VIOCamImplType iType) {
    switch (iType) {
        case VIOCamImplType::OpenCV: {
            cv::Mat cvMat;
            return std::make_unique<OpenCVFrame>(cvMat);
        }
        default:
            break;
    }
    return nullptr;
}

}  // namespace VIO