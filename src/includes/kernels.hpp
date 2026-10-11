#ifndef KERNELS_HPP
#define KERNELS_HPP

#include <vector>

//卷积核类型别名，等价于 std::vector<std::vector<int>>
using Matrix = std::vector<std::vector<int>>;

extern Matrix sobx;
extern Matrix soby;
extern Matrix gauss_kernel;   // 3x3 高斯平滑核，元素和为 gauss_kernel_sum

// 高斯核元素之和，用于卷积后归一化（除以它）
extern const int gauss_kernel_sum;

// 算法可调参数，统一在 kernels.cpp 中设置

extern const double canny_low_threshold;
extern const double canny_high_threshold;
extern const int median_default_window;

// 把数值钳位到 [0, 255]，供像素写入、梯度截断等场景复用
int clamp255(int v);

#endif
