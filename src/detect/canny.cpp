// Canny 边缘检测
//
// 两种实现：
//   canny_edge_standard —— 灰度化 -> 高斯平滑 -> cv::Canny
//   canny_edge_improved —— 灰度化 -> 3×3 高斯平滑 -> 四方向 Sobel -> NMS -> 双阈值 + BFS 连接

#include "calib/canny.hpp"
#include "calib/convolution.hpp"
#include "calib/gray.hpp"
#include "calib/kernels.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>

#include <cmath>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace calib {
namespace {


}  // namespace

Image canny_edge_improved(const Image& src, const CannyParams& params) {

    if (params.low_threshold < 0.0 ||
        params.low_threshold > params.high_threshold)
        throw std::invalid_argument("Canny: 阈值必须满足 0 <= low <= high");

    const int h = src.h;
    const int w = src.w;
    if (src.data.empty() || w <= 0 || h <= 0) return {};

    // 1. 灰度化 + 3×3 高斯平滑。
    const Matrix gray = to_gray_matrix(src);
    cv::Mat gray8(h, w, CV_8UC1);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            gray8.at<uchar>(y, x) = static_cast<uchar>(clamp255(gray[y][x]));
    cv::Mat blurred8;
    cv::GaussianBlur(gray8, blurred8, cv::Size(3, 3), 0.0);
    Matrix blurred(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            blurred[y][x] = blurred8.at<uchar>(y, x);

    // 2. 四方向梯度（命名沿用 assignment_ws：gx 实际是 ∂y，gy 实际是 ∂x）
    const Matrix gx = conv(blurred, sobx);
    const Matrix gy = conv(blurred, soby);
    const Matrix g45 = conv(blurred, sob45);
    const Matrix g135 = conv(blurred, sob135);

    std::vector<std::vector<double>> magnitude(h, std::vector<double>(w, 0.0));
    std::vector<std::vector<double>> dir(h, std::vector<double>(w, 0.0));
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const double dy = gx[y][x];
            const double dx = gy[y][x];
            const double d45 = g45[y][x], d135 = g135[y][x];
            magnitude[y][x] =
                std::sqrt(dx * dx + dy * dy + d45 * d45 + d135 * d135);

            // atan2(∂y, ∂x) 是标准梯度方向；归一到 [0, 180) 度，
            // NMS 只关心梯度方向的轴线。
            double angle = std::atan2(dy, dx) * 180.0 / M_PI;
            if (angle < 0.0) angle += 180.0;
            dir[y][x] = angle;
        }
    }

    // 3. 梯度方向非极大值抑制；最外圈没有完整邻域，直接保持 0
    std::vector<std::vector<double>> nms(h, std::vector<double>(w, 0.0));
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            const double angle = dir[y][x];
            const double m = magnitude[y][x];
            double m1 = 0.0, m2 = 0.0;
            if (angle < 22.5 || angle >= 157.5) {  // 水平
                m1 = magnitude[y][x - 1];
                m2 = magnitude[y][x + 1];
            } else if (angle < 67.5) {  // 主对角
                m1 = magnitude[y - 1][x - 1];
                m2 = magnitude[y + 1][x + 1];
            } else if (angle < 112.5) {  // 竖直
                m1 = magnitude[y - 1][x];
                m2 = magnitude[y + 1][x];
            } else {  // 副对角
                m1 = magnitude[y - 1][x + 1];
                m2 = magnitude[y + 1][x - 1];
            }
            if (m >= m1 && m >= m2) nms[y][x] = m;
        }
    }

    // 4. 双阈值：0 背景，1 强边缘，2 弱边缘候选
    std::vector<std::vector<unsigned char>> flag(
        h, std::vector<unsigned char>(w, 0));
    std::queue<std::pair<int, int>> q;
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            const double m = nms[y][x];
            if (m <= 0.0) continue;  // 被抑制的点/平坦区不参与
            if (m >= params.high_threshold) {
                flag[y][x] = 1;
                q.push({y, x});
            } else if (m >= params.low_threshold) {
                flag[y][x] = 2;
            }
        }
    }

    // 5. 从所有强边缘出发，8 邻域 BFS 把连通的弱边缘提升为强边缘
    static const int dy8[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    static const int dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    while (!q.empty()) {
        const auto [cy, cx] = q.front();
        q.pop();
        for (int k = 0; k < 8; ++k) {
            const int ny = cy + dy8[k];
            const int nx = cx + dx8[k];
            if (ny < 0 || ny >= h || nx < 0 || nx >= w) continue;
            if (flag[ny][nx] != 2) continue;
            flag[ny][nx] = 1;
            q.push({ny, nx});
        }
    }

    Image out;
    out.w = w;
    out.h = h;
    out.ch = 1;
    out.data = Matrix(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            out.data[y][x] = (flag[y][x] == 1) ? 255 : 0;
    return out;
}

Image canny_edge_standard(const Image& src,
                          const StandardCannyParams& params) {
    if (params.blur_window <= 0 || params.blur_window % 2 == 0)
        throw std::invalid_argument("Canny: blur_window 必须是正奇数");
    if (params.blur_sigma < 0.0)
        throw std::invalid_argument("Canny: blur_sigma 不能为负");
    if (params.low_threshold < 0.0 ||
        params.low_threshold > params.high_threshold)
        throw std::invalid_argument("Canny: 阈值必须满足 0 <= low <= high");
    if (params.aperture_size != 3 && params.aperture_size != 7)
        throw std::invalid_argument("Canny: aperture_size 只能是 3 或 7");

    const int h = src.h;
    const int w = src.w;
    if (src.data.empty() || w <= 0 || h <= 0) return {};

    // 灰度化 -> 高斯平滑 -> OpenCV 标准 Canny
    const Matrix gray = to_gray_matrix(src);
    cv::Mat gray8(h, w, CV_8UC1);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            gray8.at<uchar>(y, x) = static_cast<uchar>(clamp255(gray[y][x]));

    cv::Mat blurred;
    cv::GaussianBlur(gray8, blurred,
                     cv::Size(params.blur_window, params.blur_window),
                     params.blur_sigma);

    cv::Mat edges;
    cv::Canny(blurred, edges, params.low_threshold, params.high_threshold,
              params.aperture_size, true);

    Image out;
    out.w = w;
    out.h = h;
    out.ch = 1;
    out.data = Matrix(h, std::vector<int>(w, 0));
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            out.data[y][x] = edges.at<uchar>(y, x) ? 255 : 0;
    return out;
}

}  // namespace calib
