// 多通道转灰度：BT.601 系数

#include "calib/gray.hpp"

namespace calib {

Matrix to_gray_matrix(const Image& src) {
    Matrix gray(src.h, std::vector<int>(src.w, 0));
    if (src.data.empty() || src.w <= 0 || src.h <= 0 || src.ch <= 0) return gray;

    for (int y = 0; y < src.h; ++y) {
        for (int x = 0; x < src.w; ++x) {
            const int offset = x * src.ch;
            if (src.ch == 1 || src.ch == 2) {
                // 灰度图，第二个通道是 alpha，不参与灰度计算
                gray[y][x] = src.data[y][offset];
            } else {
                const int r = src.data[y][offset + 0];
                const int g = src.data[y][offset + 1];
                const int b = src.data[y][offset + 2];
                gray[y][x] = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);
            }
        }
    }
    return gray;
}

}  // namespace calib
