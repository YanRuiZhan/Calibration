#ifndef MEDIAN_BLUR_HPP
#define MEDIAN_BLUR_HPP

#include "calib/image_io.hpp"
#include "calib/kernels.hpp"

namespace calib {

// 对 Image 做 n*n 窗口的中值滤波
// 灰度(ch==1)、彩色(ch==3)统一走逐通道流程；
// 含 alpha 的图（ch==2/4）跳过 alpha 通道不处理
Image median_blur(const Image& src, int win);

}  // namespace calib

#endif
