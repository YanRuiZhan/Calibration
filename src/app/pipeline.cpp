// 图像处理流水线：串联各步骤，供 main 调用

#include "calib/pipeline.hpp"

#include <algorithm>
#include <utility>

namespace calib {

PipelineResult pipeline(const Image& raw, const CannyParams& canny,
                        const EllipseParams& ellipse) {
    PipelineResult result;
    // Canny 内部已完成 3×3 高斯平滑，这里不再重复平滑。
    result.binary = canny_edge_improved(raw, canny);
    result.candidates = detect_ellipses(result.binary, ellipse);
    result.count = classify_circles(result.candidates);
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
