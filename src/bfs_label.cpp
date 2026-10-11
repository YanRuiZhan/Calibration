// 连通域标记（手写 BFS，不依赖 OpenCV）
// 输入单通道二值图，输出每个前景连通域的编号和统计信息

#include "includes/bfs_label.hpp"
#include <kernels.hpp>

#include <algorithm>
#include <queue>
#include <utility>  // 提供std::pair
#include <vector>

LabelResult bfs_label(const Matrix& binary) {
    LabelResult result;

    // 空图直接返回，避免后续访问越界
    if (binary.empty() || binary[0].empty()) return result;

    const int h = static_cast<int>(binary.size());
    const int w = static_cast<int>(binary[0].size());

    result.h = h;
    result.w = w;
    result.label = Matrix(h, std::vector<int>(w, 0));   // 0 表示背景/未访问

    int flag = 0;   // 连通域编号，从 1 开始
    // 四邻域偏移：行方向 +-1，列方向 +-1
    const int d_row[4] = {-1, 1, 0, 0};
    const int d_col[4] = { 0, 0,-1, 1};
    std::queue<std::pair<int, int>> q;

    // STEP1. 遍历二值图，找每个尚未标记的前景连通域起点
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            if (binary[i][j] != 255 || result.label[i][j] != 0) continue;

            // 发现新连通域：编号并初始化统计
            ++flag;
            Component comp;
            comp.label = flag;
            comp.area = 0;
            comp.min_x = comp.max_x = j;   // j 是列 -> x
            comp.min_y = comp.max_y = i;   // i 是行 -> y
            long long sum_col = 0, sum_row = 0;   // 坐标和，用于求质心

            q.push({i, j});
            result.label[i][j] = flag;

            // STEP2. BFS 扩散，直到填满整个连通域
            while (!q.empty()) {
                auto [row, col] = q.front();
                q.pop();

                ++comp.area;
                sum_col += col;
                sum_row += row;
                comp.min_x = std::min(comp.min_x, col);
                comp.max_x = std::max(comp.max_x, col);
                comp.min_y = std::min(comp.min_y, row);
                comp.max_y = std::max(comp.max_y, row);

                for (int c = 0; c < 4; ++c) {
                    const int next_row = row + d_row[c];
                    const int next_col = col + d_col[c];
                    if (next_row < 0 || next_row >= h) continue;
                    if (next_col < 0 || next_col >= w) continue;
                    // 只扩散前景像素，且未访问
                    if (binary[next_row][next_col] != 255) continue;
                    if (result.label[next_row][next_col] != 0) continue;

                    result.label[next_row][next_col] = flag;
                    q.push({next_row, next_col});
                }
            }

            // 该连通域扩散完毕：计算质心并保存统计
            comp.cx = static_cast<double>(sum_col) / comp.area;
            comp.cy = static_cast<double>(sum_row) / comp.area;
            result.components.push_back(comp);
        }
    }
    return result;
}
