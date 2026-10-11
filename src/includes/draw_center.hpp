#ifndef DRAW_CENTER_HPP
#define DRAW_CENTER_HPP

#include "bfs_label.hpp"
#include "image_io.hpp"
#include <string>
#include <vector>

// 在 raw 原图上，为每个连通域中心画一个红色十字，并保存到 out_path
// raw      : 原始图像（用于作为绘制底图）
// comps    : 待标注的连通域（如 pick_circular 的结果）
// arm      : 十字线半长（像素），总长为 2*arm+1
// thickness: 红色主线线宽
// 返回是否保存成功
bool draw_center(const Image& raw,
                 const std::vector<Component>& comps,
                 const std::string& out_path,
                 int arm = 18,
                 int thickness = 2);

#endif
