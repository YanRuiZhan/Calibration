// 滤波与算法参数配置

/*
 中值滤波（一种统计排序滤波器）：
    将 n*n 范围内元素做从大到小排序后，中间的数作为滤波结果。
    实现见 median_blur.cpp 的滑动直方图版本。
 */

#include "calib/kernels.hpp"

namespace calib {

// 中值滤波默认窗口大小，必须为正奇数；可由命令行覆盖
const int median_default_window = 3;

// 把数值钳位到 [0, 255]
int clamp255(int v) {
    return v < 0 ? 0 : (v > 255 ? 255 : v);
}

}  // namespace calib
