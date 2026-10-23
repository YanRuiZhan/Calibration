#ifndef CANNY_HPP
#define CANNY_HPP

#include "calib/image_io.hpp"

namespace calib {

// 标准 Canny 参数（OpenCV cv::Canny）。
// 先做高斯平滑，再用 Sobel 求梯度、非极大值抑制和双阈值连接。
struct StandardCannyParams {
    int blur_window = 5;          // 高斯平滑窗口，正奇数
    double blur_sigma = 0.0;      // 0 表示由窗口自动推导
    double low_threshold = 30.0;  // 弱边缘阈值
    double high_threshold = 90.0; // 强边缘阈值
    int aperture_size = 3;        // Sobel 孔径，3 或 7
};

// 返回单通道二值边缘图（边缘 255，背景 0）。
// 参数非法（窗口非正奇数、阈值顺序错误）时抛出 std::invalid_argument。
Image canny_edge_standard(const Image& src,
                          const StandardCannyParams& params =
                              StandardCannyParams{});

// 改进 Canny 参数。
//
// 使用 3×3 高斯平滑、四方向（水平、垂直、45°、135°）Sobel，
// 幅值为四方向平方和开方；之后执行方向非极大值抑制、双阈值
// 和 BFS 弱边缘连接。
// 注意：四方向幅值与标准 Canny 的幅值不在同一尺度上，阈值不能直接搬用。
struct CannyParams {
    double low_threshold = 20.0;    // 弱边缘阈值
    double high_threshold = 110.0;  // 强边缘阈值
};

// 返回单通道二值边缘图（边缘 255，背景 0）。
// 参数非法（窗口非正奇数、阈值顺序错误、sigma 非正）时抛出 std::invalid_argument。
Image canny_edge_improved(const Image& src,
                          const CannyParams& params = CannyParams{});

}  // namespace calib

#endif
