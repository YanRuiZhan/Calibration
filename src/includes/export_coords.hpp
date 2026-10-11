#ifndef EXPORT_COORDS_HPP
#define EXPORT_COORDS_HPP

#include "bfs_label.hpp"
#include <string>
#include <vector>

// 把连通域中心坐标导出为 CSV 文件
// comps   : 待导出的连通域（如 pick_circular 的结果）
// out_path: 输出文件路径（.csv）
// 表头: detection_id,type,cx,cy,area,bbox_w,bbox_h
// 返回是否写入成功
bool export_coords(const std::vector<Component>& comps, const std::string& out_path);

#endif
