#pragma once
#include <string>

#include "CamImpl/CamImpl.h"

namespace VIO {
struct AppConfig {
    int  camera_id  = 0;
    int  width      = 640;
    int  height     = 480;
    int  fps        = 30;
    bool enable_ui  = true;
    int  skip_frame = 5;

    VIOCamImplType VIO_type = VIOCamImplType::OpenCV;
};
}  // namespace VIO