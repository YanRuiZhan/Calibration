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

// 按主方向把已检测到的点分行，并在每行内从左到右连续编号。
// 漏检点不占编号位；不使用单应映射，也不补出缺失点。
// 失败时清除编号，不改变中心、类别或输入顺序。
GridNumberingResult number_grid(std::vector<Component>& comps,
                                int expected_rows = 15, int expected_cols = 15);

}  // namespace calib
#endif
