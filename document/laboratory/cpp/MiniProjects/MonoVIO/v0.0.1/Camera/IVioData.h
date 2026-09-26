#pragma once

#include <stdint.h>
#include <memory>

namespace VIO {
class IFrame {
 public:
    virtual ~IFrame() = default;

    virtual int            GetWidth() const { return _width; };
    virtual int            GetHeight() const { return _height; };
    virtual const uint8_t* GetData() const { return _data; };

    virtual bool                         empty()       = 0;
    virtual std::unique_ptr<VIO::IFrame> clone() const = 0;
    // Add this to get the image for debugging/processing

 private:
    int      _width, _height;
    uint8_t* _data;
};

class IMat {
 public:
    virtual ~IMat() = default;
};

}  // namespace VIO