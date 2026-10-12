// 阈值分割（手写实现，不依赖 OpenCV）
// Otsu 全局反向二值化：最大类间方差自动选取阈值

#include "calib/threshold.hpp"

#include <vector>

namespace calib {

// ---------------- 单通道：Otsu 全局阈值 ----------------
// 返回使类间方差最大的阈值；直方图退化（全同一灰度）时返回该灰度值
static int otsu_threshold_channel(const Matrix& raw) {
    const int h = static_cast<int>(raw.size());
    const int w = static_cast<int>(raw[0].size());

    // 灰度直方图，图像读入时已 clamp 到 0~255
    std::vector<long long> hist(256, 0);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            ++hist[clamp255(raw[y][x])];

    const long long total = static_cast<long long>(h) * w;

    // 全图灰度总和，用于快速计算某阈值以上的平均灰度
    long long sum_all = 0;
    for (int v = 0; v < 256; ++v) sum_all += static_cast<long long>(v) * hist[v];

    long long w0 = 0;          // 前景（低灰度侧）像素数
    long long sum0 = 0;        // 前景灰度总和
    double best_var = -1.0;    // 最大类间方差
    int best_T = 0;

    for (int T = 0; T < 256; ++T) {
        w0 += hist[T];
        if (w0 == 0) continue;             // 前景为空，跳过
        const long long w1 = total - w0;
        if (w1 == 0) break;                // 背景为空，后面阈值只会更差

        sum0 += static_cast<long long>(T) * hist[T];

        const double mu0 = static_cast<double>(sum0) / w0;
        const double mu1 = static_cast<double>(sum_all - sum0) / w1;
        const double diff = mu0 - mu1;

        // 类间方差 σ² = w0 * w1 * (μ0 - μ1)²（用像素数代替比例，等价比较）
        const double var = static_cast<double>(w0) * static_cast<double>(w1) * diff * diff;
        if (var > best_var) {
            best_var = var;
            best_T = T;
        }
    }
    return best_T;
}

// ---------------- 对外接口（逐通道调度） ----------------

Image otsu_threshold_inv(const Image& src) {
    Image out;
    out.w = src.w;
    out.h = src.h;
    out.ch = src.ch;
    out.data = Matrix(src.h, std::vector<int>(static_cast<size_t>(src.w) * src.ch, 0));

    if (src.data.empty() || src.w <= 0 || src.h <= 0 || src.ch <= 0)
        return out;

    // ch==2/4 的最后一个通道视为 alpha，二值化后原样保留
    const bool has_alpha = (src.ch == 2 || src.ch == 4);

    Matrix channel(src.h, std::vector<int>(src.w, 0));

    for (int c = 0; c < src.ch; ++c) {
        if (has_alpha && c == src.ch - 1) {
            for (int y = 0; y < src.h; ++y)
                for (int x = 0; x < src.w; ++x)
                    out.data[y][x * src.ch + c] = src.data[y][x * src.ch + c];
            continue;
        }

        for (int y = 0; y < src.h; ++y)
            for (int x = 0; x < src.w; ++x)
                channel[y][x] = src.data[y][x * src.ch + c];

        const int T = otsu_threshold_channel(channel);

        // 反向：灰度 <= T 的黑色部分设为前景
        for (int y = 0; y < src.h; ++y)
            for (int x = 0; x < src.w; ++x)
                out.data[y][x * src.ch + c] = (channel[y][x] <= T) ? 255 : 0;
    }
    return out;
}

}  // namespace calib
