#ifndef GRID_NUMBERING_HPP
#define GRID_NUMBERING_HPP

#include "calib/component.hpp"
#include <string>
#include <vector>

namespace calib {

struct GridNumberingResult {
    bool success = false;
    int rows = 0;
    int cols = 0;
    std::string message;
};

// 当前靶标为 15×15 点阵；大圆固定在网格中心，小圆在其左侧一格，
// 小圆 -> 大圆为列正方向。候选完整时做严格检查；数量不符（板边缺点、
// 少量误检）时自动改用单应栅格容错拟合，未落到栅格上的候选保持
// row/col/grid_id = 0，由调用方决定是否剔除。
// 失败时清除编号，不改变中心、类别或输入顺序。
GridNumberingResult number_grid(std::vector<Component>& comps,
                                int expected_rows = 15, int expected_cols = 15);

}  // namespace calib
#endif
