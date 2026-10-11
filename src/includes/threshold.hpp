#ifndef THRESHOLD_HPP
#define THRESHOLD_HPP

#include "image_io.hpp"
#include "kernels.hpp"

// Otsu 反向二值化（手写，不依赖 OpenCV）
// 遍历 0~255 所有阈值，取类间方差最大者作为全局阈值 T：
//     value <= T -> 前景 255
//     否则       -> 背景 0
// 反向的含义：灰度较低的像素（黑色圆点）成为前景。
// 适合对比度好、直方图近双峰的图像。
Image otsu_threshold_inv(const Image& src);

#endif
