// 图像处理流水线：串联各步骤，供 main 调用

#include "calib/pipeline.hpp"

#include <algorithm>
#include <utility>

namespace calib {

PipelineResult pipeline(const Image& raw, const CannyParams& canny,
                        const EllipseParams& ellipse) {
    PipelineResult result;
    // Canny 边缘提取
    result.binary = canny_edge_improved(raw, canny);
    // 筛选候选椭圆
    result.candidates = detect_ellipses(result.binary, ellipse);
    // 按面积大小分类
    result.count = classify_circles(result.candidates);
    // 按主方向编号
    result.numbering = number_grid(result.candidates);

    if (result.numbering.success) {
        // 容错编号后，未落到栅格上的候选是误检，直接剔除，
        // 保证输出坐标和标注图里只剩真实圆点。
        result.candidates.erase(
            std::remove_if(result.candidates.begin(), result.candidates.end(),
                           [](const Component& c) { return c.grid_id <= 0; }),
            result.candidates.end());
        result.count = classify_circles(result.candidates);
    }
    return result;
}

}  // namespace calib
