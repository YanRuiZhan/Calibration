#ifndef EXPORT_COORDS_HPP
#define EXPORT_COORDS_HPP

#include "calib/component.hpp"
#include <string>
#include <vector>

namespace calib {

// 把圆点中心坐标导出为 CSV 文件
// comps   : 待导出的圆点候选
// out_path: 输出文件路径（.csv）
// 表头: detection_id,type,cx,cy,area,bbox_w,bbox_h
// 返回是否写入成功
bool export_coords(const std::vector<Component>& comps, const std::string& out_path);

}  // namespace calib

#endif
