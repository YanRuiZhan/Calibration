#ifndef KERNELS_HPP
#define KERNELS_HPP

#include <vector>

namespace calib {

//卷积核类型别名，等价于 std::vector<std::vector<int>>
using Matrix = std::vector<std::vector<int>>;

// 算法可调参数，统一在 kernels.cpp 中设置

// 中值滤波默认窗口大小，必须为正奇数；可由命令行覆盖
extern const int median_default_window;

// 把数值钳位到 [0, 255]，供像素写入、梯度截断等场景复用
int clamp255(int v);

}  // namespace calib

#endif
