#ifndef GRAY_HPP
#define GRAY_HPP

#include "calib/image_io.hpp"
#include "calib/kernels.hpp"

namespace calib {

// 按 BT.601 把多通道图转成单通道灰度矩阵，不修改原图。
// 1/2 通道：直接取第 0 通道（灰度，可能带 alpha）；
// 3/4 通道：Y = 0.299R + 0.587G + 0.114B。
// Canny 边缘检测使用这一份转换。
Matrix to_gray_matrix(const Image& src);

}  // namespace calib

#endif
