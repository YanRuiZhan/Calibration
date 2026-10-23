#include "calib/grid_numbering.hpp"

#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
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

GridNumberingResult fail(const std::string& message) {
    GridNumberingResult result;
    result.message = message;
    return result;
}

// 严格编号：候选数必须正好等于 rows*cols，逐行要求完整且行内间距一致。
GridNumberingResult number_grid_strict(std::vector<Component>& comps,
                                       int expected_rows, int expected_cols) {
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

// 单应栅格拟合结果。ij/residual/inlier 按输入候选下标保存，
// node 为去重后被占据的整数节点，index 为节点对应的候选下标。
struct LatticeFit {
    cv::Mat H;
    std::vector<cv::Point2i> ij;
    std::vector<double> residual;
    std::vector<bool> inlier;
    std::vector<cv::Point2i> nodes;
    std::vector<int> index;
};

// 用图像点 -> 整数栅格点的单应，把所有候选重新分配到最近节点。
void assign_by_homography(const cv::Mat& H,
                          const std::vector<cv::Point2d>& image,
                          double tolerance, LatticeFit& fit) {
    cv::Mat Hinv;
    if (cv::invert(H, Hinv) == 0.0 || Hinv.empty()) {
        fit.ij.assign(image.size(), cv::Point2i(0, 0));
        fit.residual.assign(image.size(), 1e9);
        fit.inlier.assign(image.size(), false);
        return;
    }
    std::vector<cv::Point2d> grid, predicted;
    cv::perspectiveTransform(image, grid, Hinv);

    fit.ij.resize(image.size());
    fit.residual.resize(image.size());
    fit.inlier.assign(image.size(), false);
    std::vector<cv::Point2d> rounded(image.size());
    for (size_t k = 0; k < image.size(); ++k) {
        fit.ij[k] = cv::Point2i(cvRound(grid[k].x), cvRound(grid[k].y));
        rounded[k] = cv::Point2d(fit.ij[k].x, fit.ij[k].y);
    }
    cv::perspectiveTransform(rounded, predicted, H);
    for (size_t k = 0; k < image.size(); ++k) {
        fit.residual[k] = cv::norm(image[k] - predicted[k]);
        fit.inlier[k] = fit.residual[k] < tolerance;
    }
}

// 同一节点有多个候选时只保留残差最小的，作为离群点剔除。
void keep_best_per_node(LatticeFit& fit) {
    std::vector<int> order(fit.ij.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return fit.residual[a] < fit.residual[b];
    });
    std::map<std::pair<int, int>, int> occupied;
    fit.nodes.clear();
    fit.index.clear();
    for (int k : order) {
        if (!fit.inlier[k]) continue;
        const auto key = std::make_pair(fit.ij[k].x, fit.ij[k].y);
        if (!occupied.emplace(key, k).second) continue;
        fit.nodes.push_back(fit.ij[k]);
        fit.index.push_back(k);
    }
}

// 以 (0,0)=大圆、(-1,0)=小圆 为基准，用仿射初值 + 单应迭代拟合栅格。
bool fit_lattice(const std::vector<Component>& comps, int large, int small,
                 double spacing, LatticeFit& fit) {
    const cv::Point2d origin(comps[large].cx, comps[large].cy);
    const cv::Point2d e1(origin.x - comps[small].cx,
                         origin.y - comps[small].cy);

    // 行方向：离大圆最近、与主方向近似垂直的点，长度即行间距初值。
    cv::Point2d e2;
    double best_score = std::numeric_limits<double>::infinity();
    for (size_t k = 0; k < comps.size(); ++k) {
        if (static_cast<int>(k) == large || static_cast<int>(k) == small)
            continue;
        const cv::Point2d v(comps[k].cx - origin.x, comps[k].cy - origin.y);
        const double len = cv::norm(v);
        if (len < 0.5 * spacing || len > 2.5 * spacing) continue;
        const double cosang = std::abs(v.dot(e1)) / (len * spacing);
        if (cosang > 0.45) continue;
        const double score = cosang + 0.15 * len / spacing;
        if (score < best_score) {
            best_score = score;
            e2 = v;
        }
    }
    if (!std::isfinite(best_score)) return false;
    if (e2.dot(cv::Point2d(-e1.y, e1.x)) < 0) e2 = -e2;

    const double det = e1.x * e2.y - e1.y * e2.x;
    if (std::abs(det) < 1e-9) return false;

    std::vector<cv::Point2d> image(comps.size());
    for (size_t k = 0; k < comps.size(); ++k)
        image[k] = cv::Point2d(comps[k].cx, comps[k].cy);

    // 仿射初值：把每个点解到整数 (i,j)，只保留残差足够小的作为初始内点。
    std::vector<cv::Point2f> src, dst;
    for (size_t k = 0; k < comps.size(); ++k) {
        const cv::Point2d d = image[k] - origin;
        const double i = (d.x * e2.y - d.y * e2.x) / det;
        const double j = (e1.x * d.y - e1.y * d.x) / det;
        const cv::Point2i q(cvRound(i), cvRound(j));
        const cv::Point2d pred(origin.x + q.x * e1.x + q.y * e2.x,
                               origin.y + q.x * e1.y + q.y * e2.y);
        if (cv::norm(image[k] - pred) >= 0.35 * spacing) continue;
        src.emplace_back(static_cast<float>(q.x), static_cast<float>(q.y));
        dst.emplace_back(static_cast<float>(image[k].x),
                         static_cast<float>(image[k].y));
    }
    if (src.size() < 8) return false;

    // 单应迭代：重分配 -> 按节点去重 -> 重拟合。
    for (int iter = 0; iter < 4; ++iter) {
        const cv::Mat H =
            cv::findHomography(src, dst, cv::RANSAC, 0.25 * spacing);
        if (H.empty()) return false;
        assign_by_homography(H, image, 0.25 * spacing, fit);
        keep_best_per_node(fit);
        if (fit.nodes.size() < 8) return false;

        src.clear();
        dst.clear();
        for (int k : fit.index) {
            src.emplace_back(static_cast<float>(fit.ij[k].x),
                             static_cast<float>(fit.ij[k].y));
            dst.emplace_back(static_cast<float>(image[k].x),
                             static_cast<float>(image[k].y));
        }
    }

    // 最终用全部内点做最小二乘拟合，再分配一次。
    fit.H = cv::findHomography(src, dst, 0);
    if (fit.H.empty()) return false;
    assign_by_homography(fit.H, image, 0.25 * spacing, fit);
    keep_best_per_node(fit);
    return fit.nodes.size() >= 8;
}

// 容错编号：允许板边缺点和少量误检。大小圆固定在网格中心，
// 即大圆在 (rows/2, cols/2)，小圆在其左侧一格；据此给出绝对行列号。
GridNumberingResult number_grid_tolerant(std::vector<Component>& comps,
                                         int expected_rows,
                                         int expected_cols) {
    if (comps.empty()) return fail("没有候选点");

    // 大圆、小圆都可能因板边误检而不唯一，逐个组合尝试，
    // 用栅格内点数最多的组合作为锚点，而不是按面积猜一个。
    std::vector<int> larges, smalls;
    for (size_t i = 0; i < comps.size(); ++i) {
        if (comps[i].type == CircleType::Large)
            larges.push_back(static_cast<int>(i));
        else if (comps[i].type == CircleType::Small)
            smalls.push_back(static_cast<int>(i));
    }
    if (larges.empty()) return fail("缺少大圆，无法确定方向");
    if (smalls.empty()) return fail("缺少小圆，无法确定方向");

    LatticeFit best;
    int best_large = -1;
    int best_small = -1;
    for (int large : larges)
        for (int small : smalls) {
            if (large == small) continue;
            const double spacing =
                std::hypot(comps[large].cx - comps[small].cx,
                           comps[large].cy - comps[small].cy);
            if (spacing < 4.0) continue;
            LatticeFit fit;
            if (!fit_lattice(comps, large, small, spacing, fit)) continue;
            if (fit.nodes.size() > best.nodes.size()) {
                best = std::move(fit);
                best_large = large;
                best_small = small;
            }
        }
    if (best_small < 0) return fail("无法拟合点阵栅格");

    // 锚点必须落在预期节点，否则说明这对大小圆与栅格不一致
    const cv::Point2i large_node(0, 0);
    const cv::Point2i small_node(-1, 0);
    bool large_ok = false, small_ok = false;
    for (size_t n = 0; n < best.nodes.size(); ++n) {
        if (best.index[n] == best_large) large_ok = best.nodes[n] == large_node;
        if (best.index[n] == best_small) small_ok = best.nodes[n] == small_node;
    }
    if (!large_ok || !small_ok) return fail("大小圆与栅格锚点不一致");

    const int row_center = expected_rows / 2;  // 大圆所在行（0 基）
    const int col_center = expected_cols / 2;
    int numbered = 0;
    for (size_t n = 0; n < best.nodes.size(); ++n) {
        const int row = row_center + best.nodes[n].y;
        const int col = col_center + best.nodes[n].x;
        if (row < 0 || row >= expected_rows || col < 0 || col >= expected_cols)
            continue;
        Component& c = comps[best.index[n]];
        c.row = row + 1;
        c.col = col + 1;
        c.grid_id = row * expected_cols + col + 1;
        ++numbered;
    }
    if (numbered < expected_rows * expected_cols * 3 / 5)
        return fail("栅格内点过少，编号不可靠");

    GridNumberingResult result;
    result.success = true;
    result.rows = expected_rows;
    result.cols = expected_cols;
    result.message = "编号成功（容错 " + std::to_string(numbered) + "/" +
                     std::to_string(expected_rows * expected_cols) + " 点）";
    return result;
}
}  // namespace

GridNumberingResult number_grid(std::vector<Component>& comps,
                                int expected_rows, int expected_cols) {
    for (auto& c : comps) c.row = c.col = c.grid_id = 0;
    if (expected_rows < 2 || expected_cols < 2)
        return fail("网格行列数必须至少为 2");

    // 数量完整时先走严格检查；失败或数量不符时再容错拟合。
    if (comps.size() == static_cast<size_t>(expected_rows) * expected_cols) {
        GridNumberingResult strict =
            number_grid_strict(comps, expected_rows, expected_cols);
        if (strict.success) return strict;
    }
    return number_grid_tolerant(comps, expected_rows, expected_cols);
}
}  // namespace calib
