// 按面积把候选圆分为大、中、小三类并统计数量

#include "calib/classify.hpp"

#include <algorithm>
#include <vector>

namespace calib {

CircleCount classify_circles(std::vector<Component>& comps,
                             double large_ratio,
                             double small_ratio) {
    CircleCount count;
    if (comps.empty()) return count;

    // 用面积中位数作为“中圆”基准，避免固定像素阈值受尺度影响
    std::vector<int> areas;
    areas.reserve(comps.size());
    for (const Component& c : comps) areas.push_back(c.area);

    std::vector<int> sorted = areas;
    std::sort(sorted.begin(), sorted.end());
    const double median = sorted[sorted.size() / 2];

    const double large_th = median * large_ratio;
    const double small_th = median * small_ratio;

    for (Component& c : comps) {
        if (c.area >= large_th) {
            c.type = CircleType::Large;
            ++count.large;
        } else if (c.area <= small_th) {
            c.type = CircleType::Small;
            ++count.small;
        } else {
            c.type = CircleType::Medium;
            ++count.medium;
        }
    }
    return count;
}

}  // namespace calib
