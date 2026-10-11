//滤波核配置

/*
 一、中值滤波（一种统计排序滤波器）：
    将n*n范围内元素做从大到小排序后，中间的数作为卷积运算的结果。

 二、sobel算子：
    sobx:[[-1, 0, 1]      soby:[[-1,-2,-1]
         ,[-2, 0, 2]           ,[ 0, 0, 0]
         ,[-1, 0, 1]]          ,[ 1, 2, 1]]

 三、canny算子:
    1.高斯模糊去噪（canny算子对噪声敏感）
    2.计算两个方向梯度canx，cany
    3.梯度方向上非极大值抑制（NMS）
    4.双阈值（low & high）
    5.将low-high阈值中间边线与high部分边线连接（BFS扩散）
 */

#include "includes/kernels.hpp"

//初始化sobel算子x、y方向卷积核,sobel算子固定为3*3模板
Matrix sobx = {{-1, 0, 1},{-2, 0, 2},{-1, 0, 1}};
Matrix soby = {{-1,-2,-1},{ 0, 0, 0},{ 1, 2, 1}};

// 3x3 高斯平滑核（和为 16），降低噪声引起的伪边缘，sobel 算子与 canny 算子共用
Matrix gauss_kernel = {{1, 2, 1}, {2, 4, 2}, {1, 2, 1}};
const int gauss_kernel_sum = 16;


// 中值滤波默认窗口大小，必须为正奇数；可由命令行覆盖
const int median_default_window = 3;

// 把数值钳位到 [0, 255]
int clamp255(int v) {
    return v < 0 ? 0 : (v > 255 ? 255 : v);
}
