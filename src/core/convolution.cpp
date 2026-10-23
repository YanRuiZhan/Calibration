// 二维卷积：复制边界填充 + 逐点加权求和
// 改进 Canny 的四个方向梯度都通过这里计算。

#include "calib/convolution.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace calib {
namespace {

// 复制边界填充：把最外圈像素向外复制 pad 圈，使卷积输出与原图同尺寸
Matrix replicate_padding(const Matrix& image, int kernel_size) {
    const int pad = kernel_size / 2;
    const int h = static_cast<int>(image.size());
    const int w = static_cast<int>(image[0].size());
    Matrix padded(h + 2 * pad, std::vector<int>(w + 2 * pad, 0));

    for (int i = 0; i < h + 2 * pad; ++i) {
        const int si = std::clamp(i - pad, 0, h - 1);
        for (int j = 0; j < w + 2 * pad; ++j) {
            const int sj = std::clamp(j - pad, 0, w - 1);
            padded[i][j] = image[si][sj];
        }
    }
    return padded;
}

}  // namespace

Matrix conv(const Matrix& image, const Matrix& filter) {
    if (image.empty() || image[0].empty() || filter.empty()) return {};
    const int h = static_cast<int>(image.size());
    const int w = static_cast<int>(image[0].size());
    const int k = static_cast<int>(filter.size());
    if (k % 2 == 0) return {};  // 只支持奇数边长核

    const Matrix padded = replicate_padding(image, k);
    Matrix out(h, std::vector<int>(w, 0));

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int sum = 0;
            for (int p = 0; p < k; ++p)
                for (int q = 0; q < k; ++q)
                    sum += filter[p][q] * padded[y + p][x + q];
            out[y][x] = sum;
        }
    }
    return out;
}

}  // namespace calib
