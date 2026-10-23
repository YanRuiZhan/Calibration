#ifndef ELLIPSE_HPP
#define ELLIPSE_HPP

#include "calib/component.hpp"
#include "calib/image_io.hpp"

#include <vector>

namespace calib {

// 从 Canny 边缘图提取椭圆（圆点在透视下呈椭圆）。
//
// 处理步骤：
//   findContours(RETR_LIST) -> fitEllipse -> 尺寸/长短轴比/残差筛选
//   -> 同一圆环的内外两条轮廓按中心去重
struct EllipseParams {
    // 长轴范围（像素）。下限用于滤掉纹理/反光产生的细小闭合轮廓。
    // 本项目图像为 1600×1200；普通圆点约 18~32 px，
    // 小圆在部分拍摄角度下约 12 px，因此下限取 10 px。
    // 换相机分辨率或拍摄距离时需要重新确认。
    double min_major = 10.0;
    double max_major = 70.0;

    // 短轴 / 长轴。斜拍时圆变椭圆，但仍明显比背景纹理更“圆”。
    // 本组图像中真实圆点的比例不低于 0.65，取 0.6 可滤掉拉长的反光轮廓。
    double min_axis_ratio = 0.6;

    // 轮廓点到拟合椭圆的平均偏差（像素）。断边、开口轮廓的残差会偏大。
    double max_residual = 1.5;

    // 同一圆环的内外轮廓中心几乎重合，距离小于该值时只保留一条。
    double dedupe_distance = 3.0;
};

// 返回椭圆拟合出的圆点候选。
// Component 中 cx/cy 为椭圆中心，area 为 πab/4，major/minor/angle/residual
// 保存拟合参数；label 从 1 开始，type 由 classify_circles 填充。
std::vector<Component> detect_ellipses(
    const Image& edges,
    const EllipseParams& params = EllipseParams{});

}  // namespace calib

#endif
