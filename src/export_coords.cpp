// 把连通域中心坐标导出为 CSV

#include "calib/export_coords.hpp"

#include <fstream>
#include <iomanip>

namespace calib {

bool export_coords(const std::vector<Component>& comps, const std::string& out_path) {
    std::ofstream f(out_path);
    if (!f) return false;

    f << "detection_id,type,cx,cy,area,bbox_w,bbox_h\n";
    f << std::fixed << std::setprecision(3);

    int id = 1;
    for (const Component& c : comps) {
        const int bw = c.max_x - c.min_x + 1;
        const int bh = c.max_y - c.min_y + 1;
        const char* type = "unknown";
        if (c.type == CircleType::Large) type = "large";
        else if (c.type == CircleType::Medium) type = "medium";
        else if (c.type == CircleType::Small) type = "small";
        f << id++ << ',' << type << ',' << c.cx << ',' << c.cy << ','
          << c.area << ',' << bw << ',' << bh << '\n';
    }
    return static_cast<bool>(f);
}

}  // namespace calib
