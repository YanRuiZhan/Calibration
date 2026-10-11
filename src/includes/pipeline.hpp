#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "image_io.hpp"
#include "kernels.hpp"
#include "bfs_label.hpp"
#include "pick.hpp"
#include "classify.hpp"

// 预处理流水线输出：
//   binary      : 中值滤波 + Otsu 反向二值化后的图像，前景 255
//   labels      : 对 binary 做 BFS 连通域标记的结果
//   candidates  : 标记结果中按形状筛选出的圆形靶点候选（已标注大/中/小类别）
//   count       : 大、中、小圆的数量统计
struct PipelineResult {
    Image binary;
    LabelResult labels;
    std::vector<Component> candidates;
    CircleCount count;
};

// 预处理流水线：中值滤波去噪 -> Otsu 反向二值化 -> BFS 连通域标记 -> 圆形筛选
// 输入: raw  原始图像（灰度或彩色），win 中值滤波窗口（正奇数）
// win <= 0 或为偶数时，跳过中值滤波，直接做 Otsu 二值化
PipelineResult pipeline(const Image& raw, int win);

#endif
