#ifndef CLASSIFY_HPP
#define CLASSIFY_HPP

#include "bfs_label.hpp"
#include <vector>

// 各类圆形数量统计
struct CircleCount {
    int large = 0;
    int medium = 0;
    int small = 0;
};

// 按面积把候选圆分成大、中、小三类
// 以所有候选的面积中位数为“中圆”基准：
//     area >= median * large_ratio        -> 大圆
//     area <= median * small_ratio        -> 小圆
//     其余                                -> 中圆
// 中圆通常数量最多，用中位数作基准比固定像素阈值更抗尺度变化。
// 参数:
//   comps       : 候选圆（会被就地修改，写入 type 字段）
//   large_ratio : 大圆面积相对中位数的倍数，默认 1.6
//   small_ratio : 小圆面积相对中位数的比例，默认 0.6
// 返回各类数量统计
CircleCount classify_circles(std::vector<Component>& comps,
                             double large_ratio = 1.6,
                             double small_ratio = 0.6);

#endif
