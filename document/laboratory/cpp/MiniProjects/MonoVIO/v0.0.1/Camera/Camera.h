#pragma once

#include "IVioData.h"

namespace VIO {

class Camera {
 public:
    virtual ~Camera(){};

    virtual bool Grab(IFrame& iframe) = 0;

 private:
};

// class SingleCamera {
//  public:
//     static SingleCamera Create(int id, int w, int h, int fps);
//     static SingleCamera GetIstance() { return _instance; };

//     virtual bool Grap(IFrame& iframe){};

//  protected:
//     SingleCamera(int id, int w, int h, int fps);
//     virtual ~SingleCamera(){};

//  private:
//     SingleCamera(const SingleCamera&) = default;
//     SingleCamera&       operator=(const SingleCamera&) = default;
//     static SingleCamera _instance;
// };

}  // namespace VIO

// TODO
// static VIO::SingleCamera Create(int id, int w, int h, int fps) {
//     static SingleCamera instance = new SingleCamera(id, w, h, fps);  //
//     inherit _instance                    = instance; return _instance;
// };