// 连通域挑选：按面积、宽高比、填充率挑出接近圆形的靶点

#include "calib/pick.hpp"

namespace calib {

std::vector<Component> pick_circular(const std::vector<Component>& comps,
                                     const PickParams& params) {
    std::vector<Component> kept;
    kept.reserve(comps.size());

    for (const Component& c : comps) {
        // 面积范围：下限排噪点，上限排板外背景和板框
        if (c.area < params.min_area || c.area > params.max_area) continue;

        // 外接矩形宽高（+1 才是像素个数）
        const int bw = c.max_x - c.min_x + 1;
        const int bh = c.max_y - c.min_y + 1;
        if (bw <= 0 || bh <= 0) continue;

        // 宽高比：斜拍后圆变椭圆，阈值不能过严
        const double aspect = static_cast<double>(bw) / bh;
        if (aspect < params.min_aspect || aspect > params.max_aspect) continue;

        // 填充率：实心圆约 π/4 ≈ 0.785，细长或空心轮廓明显偏低
        const double fill = static_cast<double>(c.area) / (bw * bh);
        if (fill < params.min_fill || fill > params.max_fill) continue;

        kept.push_back(c);
    }
    return kept;
}

}  // namespace calib
