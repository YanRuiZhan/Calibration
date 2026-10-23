#ifndef COMPONENT_HPP
#define COMPONENT_HPP

namespace calib {

// 圆形尺寸类别
enum class CircleType { Unknown, Small, Medium, Large };

// 单个圆点候选：中心、椭圆参数和网格编号
struct Component {
    int label = 0;          // 候选序号，从 1 开始
    int area = 0;           // 椭圆面积 πab/4
    int min_x = 0, min_y = 0;   // 外接矩形左上角
    int max_x = 0, max_y = 0;   // 外接矩形右下角
    double cx = 0.0, cy = 0.0;  // 椭圆拟合中心（像素坐标）
    double major = 0.0;         // 长轴（像素）
    double minor = 0.0;         // 短轴（像素）
    double angle = 0.0;         // 长轴相对水平方向的角度（度）
    double residual = 0.0;      // 轮廓点到拟合椭圆的平均偏差（像素）
    int row = 0, col = 0, grid_id = 0;  // 从 1 开始；0 表示未编号
    CircleType type = CircleType::Unknown;   // 大/中/小圆类别
};

}  // namespace calib

#endif
