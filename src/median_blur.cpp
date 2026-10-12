//中值平滑滤波

#include "calib/median_blur.hpp"
#include "calib/kernels.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace calib {

// 插入排序法找中间值，排序后直接返回中位数
/*时间复杂度 O(n^2)，n 为窗口内元素个数
  空间复杂度 O(1)，原地排序，无需额外分配
  窗口较小时（如 3x3、5x5）常数因子小、缓存友好*/
/*
int insertion_sort(std::vector<int>& arry) {
    for (std::size_t i = 1; i < arry.size(); ++i) {
        int key = arry[i];
        std::size_t j = i - 1;
        // j 为无符号，用 j>0 时比较 arry[j-1]，避免 j-1 下溢
        while (j > 0 && arry[j - 1] > key) {
            arry[j] = arry[j - 1];
            --j;
        }
        arry[j] = key;
    }
    return arry[arry.size() / 2];
}
*/

// 滑动直方图滤波（单通道）：整幅图只在列方向做 O(1) 分摊的增量更新
// 时间复杂度 O(H*W*k)，远优于逐像素全排序的 O(H*W*k^2)
// len_win 必须是正奇数，否则原样返回
Matrix sliding_hist_sort(const Matrix& raw, int len_win) {
    if (raw.empty() || raw[0].empty()) return {};
    if (len_win <= 0 || len_win % 2 == 0) return raw;

    const int h = static_cast<int>(raw.size());
    const int w = static_cast<int>(raw[0].size());
    const int pad = len_win / 2;
    const int median_rank = len_win * len_win / 2;   // 排序后中位数所在下标

    Matrix img(h, std::vector<int>(w, 0));

    // 边界采用复制填充：坐标越界时钳位到最近的边界像素
    auto at = [&](int y, int x) -> int {
        if (y < 0) y = 0; else if (y > h - 1) y = h - 1;
        if (x < 0) x = 0; else if (x > w - 1) x = w - 1;
        return raw[y][x];
    };

    // 直方图用定长数组统计 0~255（图像读入即 clamp 到该范围）
    std::vector<int> hist(256, 0);

    for (int i = 0; i < h; ++i) {
        // 每换一行，用本行首个窗口重建直方图
        std::fill(hist.begin(), hist.end(), 0);
        for (int p = i - pad; p <= i + pad; ++p)
            for (int q = -pad; q <= pad; ++q)
                ++hist[at(p, q)];

        // 找中位数：从 0 累加计数直到跨过 median_rank
        auto find_median = [&]() -> int {
            int sum = 0;
            for (int v = 0; v < 256; ++v) {
                sum += hist[v];
                if (sum > median_rank) return v;
            }
            return 255;
        };

        img[i][0] = find_median();

        // 沿行向右滑动窗口：每步移除最左列、加入最右列
        for (int j = 1; j < w; ++j) {
            int out_col = j - pad - 1;   // 离开窗口的列
            int in_col = j + pad;        // 进入窗口的列
            // 将列索引钳位到有效范围，防止越界
            int clamped_out_col =
                out_col < 0 ? 0 : (out_col > w - 1 ? w - 1 : out_col);
            int clamped_in_col =
                in_col < 0 ? 0 : (in_col > w - 1 ? w - 1 : in_col);

            //逐行减去和添加元素
            for (int y = i - pad; y <= i + pad; ++y) {
                // 将行索引钳位到有效范围，防止越界
                int clamped_row = y < 0 ? 0 : (y > h - 1 ? h - 1 : y);
                --hist[raw[clamped_row][clamped_out_col]];
                ++hist[raw[clamped_row][clamped_in_col]];
            }
            img[i][j] = find_median();
        }
    }
    return img;
}


// 对单通道进行中值滤波，模板窗口大小为 len_win
Matrix median_blur_channel(const Matrix& raw, int len_win) {
    return sliding_hist_sort(raw, len_win);
}

// 对多通道图像进行中值滤波
// 若图像含 alpha（ch==2 或 ch==4），只对颜色通道滤波，
// alpha 通道原样保留，避免半透明边缘被改变
Image median_blur(const Image& raw, int len_win) {
    Image img;
    img.w = raw.w;
    img.h = raw.h;
    img.ch = raw.ch;
    img.data = Matrix(raw.h, std::vector<int>(raw.w * raw.ch, 0));

    // ch==2/4 的第 ch-1 通道视为 alpha
    const bool has_alpha = (raw.ch == 2 || raw.ch == 4);

    // 逐通道拆分 -> 滤波 -> 合回，避免跨通道串味
    // ch==1（灰度）时循环只跑一次
    Matrix channel(raw.h, std::vector<int>(raw.w, 0));

    for (int c = 0; c < raw.ch; ++c) {
        // alpha 通道是第 raw.ch-1 通道，这里直接原样拷贝，不做滤波
        if (has_alpha && c == raw.ch - 1) {
            for (int y = 0; y < raw.h; ++y)
                for (int x = 0; x < raw.w; ++x)
                    img.data[y][x * raw.ch + c] = raw.data[y][x * raw.ch + c];
            }

        // 拆出第 c 通道
        for (int y = 0; y < raw.h; ++y)
            for (int x = 0; x < raw.w; ++x)
                channel[y][x] = raw.data[y][x * raw.ch + c];
        Matrix filtered = median_blur_channel(channel, len_win);

        // 合回第 c 通道
        for (int y = 0; y < raw.h; ++y)
            for (int x = 0; x < raw.w; ++x)
                img.data[y][x * raw.ch + c] = filtered[y][x];
    }
    return img;
}

}  // namespace calib
