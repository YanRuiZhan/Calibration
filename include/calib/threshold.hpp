#ifndef THRESHOLD_HPP
#define THRESHOLD_HPP

#include "calib/image_io.hpp"
#include "calib/kernels.hpp"

namespace calib {

// Otsu 反向二值化（OpenCV cv::threshold）
// 逐通道自动选取全局阈值 T，alpha 通道原样保留：
//     value <= T -> 前景 255
//     否则       -> 背景 0
// 反向的含义：灰度较低的像素（黑色圆点）成为前景。
// 适合对比度好、直方图近双峰的图像。
Image otsu_threshold_inv(const Image& src);

}  // namespace calib

#endif
