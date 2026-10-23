#include "calib/grid_numbering.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace calib {
namespace {

struct Point {
    double u;
    double v;
};

struct Row {
    std::vector<int> ids;
    double slope = 0.0;
    double intercept = 0.0;
};

bool fit_row(const std::vector<Point>& points, Row& row) {
    if (row.ids.size() < 2) return false;

    double mean_u = 0.0;
    double mean_v = 0.0;
    for (int id : row.ids) {
        mean_u += points[id].u;
        mean_v += points[id].v;
    }
    mean_u /= row.ids.size();
    mean_v /= row.ids.size();

    double uu = 0.0;
    double uv = 0.0;
    for (int id : row.ids) {
        const double du = points[id].u - mean_u;
        uu += du * du;
        uv += du * (points[id].v - mean_v);
    }
    if (uu < 1e-12) return false;

    row.slope = uv / uu;
    row.intercept = mean_v - row.slope * mean_u;
    return true;
}

GridNumberingResult fail(const std::string& message) {
    GridNumberingResult result;
    result.message = message;
    return result;
}

}  // namespace

GridNumberingResult number_grid(std::vector<Component>& comps,
                                int expected_rows, int expected_cols) {
    for (auto& c : comps) c.row = c.col = c.grid_id = 0;
    if (expected_rows < 2 || expected_cols < 2)
        return fail("网格行列数必须至少为 2");
    if (comps.empty()) return fail("没有候选点");

    std::vector<int> large_ids;
    std::vector<int> small_ids;
    for (size_t i = 0; i < comps.size(); ++i) {
        if (comps[i].type == CircleType::Large)
            large_ids.push_back(static_cast<int>(i));
        else if (comps[i].type == CircleType::Small)
            small_ids.push_back(static_cast<int>(i));
    }
    if (large_ids.empty() || small_ids.empty())
        return fail("缺少大圆或小圆，无法确定方向");

    int large = -1;
    int small = -1;
    double best_anchor_score = std::numeric_limits<double>::infinity();
    for (int l : large_ids) {
        for (int s : small_ids) {
            const double distance = std::hypot(comps[l].cx - comps[s].cx,
                                               comps[l].cy - comps[s].cy);
            const double area_ratio = comps[l].area > 0
                ? static_cast<double>(comps[s].area) / comps[l].area : 1.0;
            if (distance < 4.0 || area_ratio >= 0.8) continue;

            // 大圆与小圆应是相邻标记点；排除远离靶标的背景大圆。
            const double score = std::abs(distance - 45.0);
            if (score < best_anchor_score) {
                large = l;
                small = s;
                best_anchor_score = score;
            }
        }
    }
    if (large < 0 || small < 0 || best_anchor_score > 15.0)
        return fail("无法找到间距符合点阵尺度的大圆和小圆锚点");

    const double dx = comps[large].cx - comps[small].cx;
    const double dy = comps[large].cy - comps[small].cy;
    const double spacing = std::hypot(dx, dy);
    if (spacing < 1e-6) return fail("大小圆中心重合");

    std::vector<Point> points;
    points.reserve(comps.size());
    for (const auto& c : comps) {
        const double x = c.cx - comps[large].cx;
        const double y = c.cy - comps[large].cy;
        points.push_back({(x * dx + y * dy) / spacing,
                          (-x * dy + y * dx) / spacing});
    }

    const double tolerance = spacing * 0.15;
    const size_t min_row_points =
        static_cast<size_t>(std::max(2, expected_cols / 2));
    std::vector<bool> used(points.size(), false);
    std::vector<Row> rows;
    rows.reserve(expected_rows);

    for (int r = 0; r < expected_rows; ++r) {
        Row best;
        double best_error = std::numeric_limits<double>::infinity();
        for (size_t i = 0; i < points.size(); ++i) {
            if (used[i]) continue;
            for (size_t j = i + 1; j < points.size(); ++j) {
                if (used[j]) continue;
                const double du = points[j].u - points[i].u;
                if (std::abs(du) < spacing * 2.0) continue;
                const double slope = (points[j].v - points[i].v) / du;
                if (std::abs(slope) > 0.30) continue;
                const double intercept = points[i].v - slope * points[i].u;

                Row candidate;
                for (size_t k = 0; k < points.size(); ++k) {
                    // 点阵约为 15×15；远离锚点的边缘候选不参与分行，
                    // 但仍保留为未编号候选，成功后由 pipeline 清除。
                    if (std::abs(points[k].u) > spacing * 9.0 ||
                        std::abs(points[k].v) > spacing * 9.0)
                        continue;
                    if (!used[k] &&
                        std::abs(points[k].v - slope * points[k].u - intercept) <
                            tolerance)
                        candidate.ids.push_back(static_cast<int>(k));
                }
                if (candidate.ids.size() < min_row_points ||
                    !fit_row(points, candidate))
                    continue;

                double error = 0.0;
                bool valid = true;
                for (int id : candidate.ids) {
                    const double residual = points[id].v -
                        candidate.slope * points[id].u - candidate.intercept;
                    if (std::abs(residual) > tolerance) {
                        valid = false;
                        break;
                    }
                    error += residual * residual;
                }
                if (valid && (candidate.ids.size() > best.ids.size() ||
                    (candidate.ids.size() == best.ids.size() && error < best_error))) {
                    best = std::move(candidate);
                    best_error = error;
                }
            }
        }
        if (best.ids.empty()) return fail("无法可靠分出点阵行");
        for (int id : best.ids) used[id] = true;
        rows.push_back(std::move(best));
    }

    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        return a.intercept < b.intercept;
    });

    int next_grid_id = 1;
    bool anchors_adjacent = false;
    for (int r = 0; r < expected_rows; ++r) {
        Row& row = rows[r];
        std::sort(row.ids.begin(), row.ids.end(), [&](int a, int b) {
            return points[a].u < points[b].u;
        });

        for (size_t i = 1; i < row.ids.size(); ++i) {
            const auto& a = points[row.ids[i - 1]];
            const auto& b = points[row.ids[i]];
            const double gap = std::hypot(b.u - a.u, b.v - a.v);
            if (gap < spacing * 0.45 || gap > spacing * 6.0)
                return fail("行内点距异常，无法可靠连续编号");
        }

        for (size_t col = 0; col < row.ids.size(); ++col) {
            const int id = row.ids[col];
            comps[id].row = r + 1;
            comps[id].col = static_cast<int>(col) + 1;
            comps[id].grid_id = next_grid_id++;
            if (id == small && col + 1 < row.ids.size() &&
                row.ids[col + 1] == large)
                anchors_adjacent = true;
        }
    }

    if (!anchors_adjacent)
        return fail("大小圆未按预期在同一行相邻");

    GridNumberingResult result;
    result.success = true;
    result.rows = expected_rows;
    result.cols = expected_cols;
    result.message = "编号成功（检测点连续编号，共 " +
                     std::to_string(next_grid_id - 1) + " 点）";
    return result;
}

}  // namespace calib
