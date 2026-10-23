// 通用像素工具

#include "calib/kernels.hpp"

namespace calib {

// 把数值钳位到 [0, 255]
int clamp255(int v) {
    return v < 0 ? 0 : (v > 255 ? 255 : v);
}

// 四个方向的 Sobel 核。
// sobx / soby 是标准水平、竖直差分；sob45 / sob135 是把核旋转 45°、135° 得到的。
// 命名沿用 assignment_ws 的约定：sobx 实际求 ∂y，soby 实际求 ∂x。
const Matrix sobx = {{-1, -2, -1},
                      {0, 0, 0},
                      {1, 2, 1}};

const Matrix soby = {{-1, 0, 1},
                     {-2, 0, 2},
                     {-1, 0, 1}};

const Matrix sob45 = {{-2, -1, 0},
                      {-1, 0, 1},
                      {0, 1, 2}};

const Matrix sob135 = {{0, 1, 2},
                      {-1, 0, 1},
                      {-2, -1, 0}};

}  // namespace calib
