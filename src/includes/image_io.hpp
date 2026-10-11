#ifndef IMAGE_IO_HPP
#define IMAGE_IO_HPP

#include "kernels.hpp"
#include <string>

// 图片结构体：data 存像素，w/h/ch 保存元信息
// Matrix 本身是"行×列"的裸二维数组
// 无法区分灰度和彩色，把尺寸带出来才能正确存取
struct Image {
    int w = 0, h = 0, ch = 0;
    Matrix data;   // 即 std::vector<std::vector<int>>
};

Image read_image(const std::string& img_path);
bool save_image(const std::string& img_path, const Image& img);

#endif
