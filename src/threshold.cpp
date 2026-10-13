// 阈值分割（OpenCV 实现）
// Otsu 全局反向二值化：最大类间方差自动选取阈值

#include "calib/threshold.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <vector>

namespace calib {

Image otsu_threshold_inv(const Image& src) {
    Image out;
    out.w = src.w;
    out.h = src.h;
    out.ch = src.ch;

    if (src.data.empty() || src.w <= 0 || src.h <= 0 || src.ch <= 0)
        return out;

    out.data = Matrix(src.h, std::vector<int>(static_cast<size_t>(src.w) * src.ch, 0));

    // ch==2/4 的最后一个通道视为 alpha，二值化后原样保留
    const bool has_alpha = (src.ch == 2 || src.ch == 4);

    // OpenCV 的 Otsu 需要单通道输入，保持原有的逐通道处理方式。
    cv::Mat channel(src.h, src.w, CV_8UC1);
    cv::Mat binary;

    for (int c = 0; c < src.ch; ++c) {
        if (has_alpha && c == src.ch - 1) {
            for (int y = 0; y < src.h; ++y)
                for (int x = 0; x < src.w; ++x)
                    out.data[y][x * src.ch + c] = src.data[y][x * src.ch + c];
            continue;
        }

        for (int y = 0; y < src.h; ++y)
            for (int x = 0; x < src.w; ++x)
                channel.at<unsigned char>(y, x) = static_cast<unsigned char>(
                    clamp255(src.data[y][x * src.ch + c]));

        cv::threshold(channel, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

        for (int y = 0; y < src.h; ++y)
            for (int x = 0; x < src.w; ++x)
                out.data[y][x * src.ch + c] = binary.at<unsigned char>(y, x);
    }
    return out;
}

}  // namespace calib
