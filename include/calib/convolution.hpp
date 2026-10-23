#ifndef CONVOLUTION_HPP
#define CONVOLUTION_HPP

#include "calib/kernels.hpp"

namespace calib {

// 二维卷积：[k/2, k/2] 边界原位填充（复制最外圈像素），输出与输入同尺寸。
// 改进 Canny 的四个方向梯度都通过这里计算。
// 只支持奇数边长核；尺寸不合法时返回空矩阵。
Matrix conv(const Matrix& image, const Matrix& filter);

}  // namespace calib

#endif
