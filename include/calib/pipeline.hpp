#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "calib/canny.hpp"
#include "calib/classify.hpp"
#include "calib/component.hpp"
#include "calib/ellipse.hpp"
#include "calib/grid_numbering.hpp"
#include "calib/image_io.hpp"

#include <vector>

namespace calib {

// 流水线输出：
//   binary      : 改进 Canny 边缘图
//   candidates  : 圆点候选（已写入大/中/小类别和编号）
//   count       : 分类计数
//   numbering   : 网格编号结果
struct PipelineResult {
    Image binary;
    std::vector<Component> candidates;
    CircleCount count;
    GridNumberingResult numbering;
};

// 原图 -> 改进 Canny（3×3 高斯平滑 + 四方向 Sobel）-> findContours/fitEllipse
//      -> 分类 -> 主方向编号。
// 参数非法时抛出 std::invalid_argument。
PipelineResult pipeline(const Image& raw,
                        const CannyParams& canny = CannyParams{},
                        const EllipseParams& ellipse = EllipseParams{});

}  // namespace calib

#endif
