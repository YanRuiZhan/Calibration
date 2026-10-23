#ifndef KERNELS_HPP
#define KERNELS_HPP

#include <vector>

namespace calib {

// 二维整数矩阵，等价于 std::vector<std::vector<int>>
using Matrix = std::vector<std::vector<int>>;

// 把数值钳位到 [0, 255]，供像素写入复用
int clamp255(int v);

// 改进 Canny 使用的四个方向 Sobel 核（3 × 3）。
// 命名沿用 assignment_ws：sobx 求的是 ∂y，soby 求的是 ∂x。
extern const Matrix sobx;
extern const Matrix soby;
extern const Matrix sob45;
extern const Matrix sob135;

}  // namespace calib

#endif
