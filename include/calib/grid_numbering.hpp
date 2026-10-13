#ifndef GRID_NUMBERING_HPP
#define GRID_NUMBERING_HPP

#include "calib/bfs_label.hpp"
#include <string>
#include <vector>

namespace calib {

struct GridNumberingResult {
    bool success = false;
    int rows = 0;
    int cols = 0;
    std::string message;
};

// 当前靶标为完整 15×15 点阵；大小圆必须在同一行且相邻。
// 小圆 -> 大圆为列正方向，图像中的顺时针侧为行正方向。
// 失败时清除编号，不改变质心、类别或输入顺序。
GridNumberingResult number_grid(std::vector<Component>& comps,
                                int expected_rows = 15, int expected_cols = 15);

}  // namespace calib
#endif
