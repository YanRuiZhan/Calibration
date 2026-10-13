#include "calib/grid_numbering.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace calib {
namespace {
struct Point { double u, v; };
struct Row {
    std::vector<int> ids;
    double slope = 0.0, intercept = 0.0;
};

bool fit_row(const std::vector<Point>& points, Row& row) {
    double u = 0.0, v = 0.0;
    for (int i : row.ids) { u += points[i].u; v += points[i].v; }
    if (row.ids.size() < 2) return false;
    u /= row.ids.size(); v /= row.ids.size();
    double uu = 0.0, uv = 0.0;
    for (int i : row.ids) {
        const double du = points[i].u - u;
        uu += du * du;
        uv += du * (points[i].v - v);
    }
    if (uu < 1e-12) return false;
    row.slope = uv / uu;
    row.intercept = v - row.slope * u;
    return true;
}
}

GridNumberingResult number_grid(std::vector<Component>& comps,
                                int expected_rows, int expected_cols) {
    for (auto& c : comps) c.row = c.col = c.grid_id = 0;
    auto fail = [](const std::string& message) {
        GridNumberingResult result;
        result.message = message;
        return result;
    };
    if (expected_rows < 2 || expected_cols < 2)
        return fail("网格行列数必须至少为 2");
    if (comps.size() != static_cast<size_t>(expected_rows) * expected_cols)
        return fail("候选数量与完整网格不符，停止编号（检查漏检/误检或网格配置）");
    int large = -1, small = -1;
    for (size_t i = 0; i < comps.size(); ++i) {
        if (comps[i].type == CircleType::Large) {
            if (large >= 0) return fail("检测到多个大圆，无法确定方向");
            large = static_cast<int>(i);
        }
        if (comps[i].type == CircleType::Small) {
            if (small >= 0) return fail("检测到多个小圆，无法确定方向");
            small = static_cast<int>(i);
        }
    }
    if (large < 0 || small < 0) return fail("缺少唯一的大圆或小圆");
    const double dx = comps[large].cx - comps[small].cx;
    const double dy = comps[large].cy - comps[small].cy;
    const double spacing = std::hypot(dx, dy);
    if (spacing < 1e-6) return fail("大小圆中心重合");
    std::vector<Point> points;
    for (const auto& c : comps) {
        const double x = c.cx - comps[large].cx;
        const double y = c.cy - comps[large].cy;
        points.push_back({(x * dx + y * dy) / spacing,
                          (-x * dy + y * dx) / spacing});
    }

    // 从主方向附近的点对枚举行直线；各行独立拟合，不强制平行。
    const double tolerance = spacing * 0.15;
    std::vector<bool> used(points.size(), false);
    std::vector<Row> rows;
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
                for (size_t k = 0; k < points.size(); ++k)
                    if (!used[k] && std::abs(points[k].v - slope * points[k].u - intercept) < tolerance)
                        candidate.ids.push_back(static_cast<int>(k));
                if (candidate.ids.size() != static_cast<size_t>(expected_cols) ||
                    !fit_row(points, candidate)) continue;
                double error = 0.0;
                bool valid = true;
                for (int k : candidate.ids) {
                    const double residual = points[k].v - candidate.slope * points[k].u - candidate.intercept;
                    if (std::abs(residual) > tolerance) { valid = false; break; }
                    error += residual * residual;
                }
                if (valid && error < best_error) { best = candidate; best_error = error; }
            }
        }
        if (best.ids.empty()) return fail("无法分出完整的共线圆点行，检查透视、畸变或误检");
        for (int i : best.ids) used[i] = true;
        rows.push_back(best);
    }
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
        return a.intercept < b.intercept;
    });
    for (auto& row : rows) {
        std::sort(row.ids.begin(), row.ids.end(), [&](int a, int b) { return points[a].u < points[b].u; });
        for (size_t c = 1; c < row.ids.size(); ++c) {
            const auto& a = points[row.ids[c - 1]];
            const auto& b = points[row.ids[c]];
            const double gap = std::hypot(b.u - a.u, b.v - a.v);
            if (gap < spacing * 0.55 || gap > spacing * 1.65)
                return fail("行内间距异常，可能存在漏点或错误分行");
        }
    }
    // 相邻行在点阵左右两端均须保持顺序，避免交叉行误编号。
    double min_u = points.front().u, max_u = min_u;
    for (const auto& p : points) { min_u = std::min(min_u, p.u); max_u = std::max(max_u, p.u); }
    for (size_t r = 1; r < rows.size(); ++r)
        for (double u : {min_u, max_u}) {
            const double gap = (rows[r].slope - rows[r - 1].slope) * u
                             + rows[r].intercept - rows[r - 1].intercept;
            if (gap < spacing * 0.45 || gap > spacing * 1.8)
                return fail("行间距异常或行直线交叉，停止编号");
        }
    // 列也必须近似共线，防止数量正确但行内错位。
    for (int c = 0; c < expected_cols; ++c) {
        std::vector<Point> swapped;
        Row column;
        for (const auto& row : rows) {
            const auto& p = points[row.ids[c]];
            column.ids.push_back(static_cast<int>(swapped.size()));
            swapped.push_back({p.v, p.u});
        }
        if (!fit_row(swapped, column)) return fail("列拟合退化");
        for (const auto& p : swapped)
            if (std::abs(p.v - column.slope * p.u - column.intercept) > tolerance)
                return fail("列共线检查失败，可能存在错位");
    }
    bool anchors_valid = false;
    for (const auto& row : rows)
        for (int c = 1; c < expected_cols; ++c)
            if (row.ids[c - 1] == small && row.ids[c] == large) anchors_valid = true;
    if (!anchors_valid) return fail("大小圆不在同一行相邻位置");

    for (int r = 0; r < expected_rows; ++r)
        for (int c = 0; c < expected_cols; ++c) {
            auto& point = comps[rows[r].ids[c]];
            point.row = r + 1;
            point.col = c + 1;
            point.grid_id = r * expected_cols + c + 1;
        }
    return {true, expected_rows, expected_cols, "编号成功"};
}
}  // namespace calib
