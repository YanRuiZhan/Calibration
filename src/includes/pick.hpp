#ifndef PICK_HPP
#define PICK_HPP

#include "bfs_label.hpp"

// 连通域挑选参数（单位：像素）
// 默认值依据图像 1 的实测分布设置：
//   普通圆点 area≈320~365，大圆 area≈784，aspect≈0.87~1.0，fill≈0.55~0.83
//   需排除的板框大块 area≈5331 aspect≈0.70 fill≈0.48
//   需排除的板边框   area≈437  aspect≈0.62 fill≈0.51
// 换图像或换尺度时需要重新确认这些阈值
struct PickParams {
    int    min_area = 100;        // 面积下限，排除噪点
    int    max_area = 1000;       // 面积上限，排除板框大块
    double min_aspect = 0.75;     // 外接矩形 宽高比下限
    double max_aspect = 1.35;     // 宽高比上限
    double min_fill = 0.60;       // 填充率下限 area/(w*h)
    double max_fill = 1.0;        // 填充率上限
};

// 从连通域列表中挑出形状接近圆形/椭圆的成分
// 保留满足面积、宽高比、填充率条件的连通域，返回子集
std::vector<Component> pick_circular(const std::vector<Component>& comps,
                                     const PickParams& params);

#endif
