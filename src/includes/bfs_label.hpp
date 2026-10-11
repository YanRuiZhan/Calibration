#ifndef BFS_LABEL_HPP
#define BFS_LABEL_HPP

#include "image_io.hpp"
#include "kernels.hpp"
#include <vector>

// 圆形尺寸类别
enum class CircleType { Unknown, Small, Medium, Large };

// 单个连通域的统计信息
struct Component {
    int label = 0;          // 连通域编号，从 1 开始
    int area = 0;           // 像素个数
    int min_x = 0, min_y = 0;   // 外接矩形左上角
    int max_x = 0, max_y = 0;   // 外接矩形右下角
    double cx = 0.0, cy = 0.0;  // 质心（像素坐标）
    CircleType type = CircleType::Unknown;   // 大/中/小圆类别
};

// 标注结果：label 与输入同尺寸，0 表示背景/未标记，>0 表示连通域编号
struct LabelResult {
    int w = 0, h = 0;
    Matrix label;   // label[y][x]
    std::vector<Component> components;      // 各连通域统计，下标 0 对应 label 1
};

// 对单通道二值图做四邻域 BFS 连通域标记
// 输入 binary: 单通道，前景为 255、背景为 0（如 pipeline 的输出，需先取通道）
// 输出: 每个前景连通域的编号与统计信息
LabelResult bfs_label(const Matrix& binary);

#endif
