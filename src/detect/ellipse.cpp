// 从 Canny 边缘图提取椭圆并换算成圆点候选

#include "calib/ellipse.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace calib {
namespace {

struct Candidate {
    // 用来存放拟合得到的椭圆中心/长短轴/角度
    cv::RotatedRect ellipse;
    // 拟合残差
    double residual = 0.0;
    int points = 0;
};

// 轮廓点到拟合椭圆的平均偏差。
// 先把点转到椭圆自身坐标系，用 (u/A)^2 + (v/B)^2 = 1 判断距离，
// 再乘平均半径近似换回像素单位。断边、开口弧的残差会明显偏大。
double fit_residual(const std::vector<cv::Point>& contour,
                    const cv::RotatedRect& ell) {
    const double ang = ell.angle * CV_PI / 180.0;
    const double ca = std::cos(ang), sa = std::sin(ang);
    const double a = ell.size.width * 0.5;
    const double b = ell.size.height * 0.5;
    if (a <= 0.0 || b <= 0.0) return 1e9;

    double sum = 0.0;
    for (const cv::Point& p : contour) {
        const double dx = p.x - ell.center.x;
        const double dy = p.y - ell.center.y;
        const double u = dx * ca + dy * sa;
        const double v = -dx * sa + dy * ca;
        const double d = std::sqrt((u * u) / (a * a) + (v * v) / (b * b));
        sum += (d - 1.0) * (d - 1.0);
    }
    return std::sqrt(sum / contour.size()) * (a + b) * 0.5;
}

}  // namespace

std::vector<Component> detect_ellipses(const Image& edges,
                                       const EllipseParams& params) {
    std::vector<Component> result;
    if (edges.data.empty() || edges.w <= 0 || edges.h <= 0) return result;

    // 内部 Image 是逐像素多通道排列；边缘图固定为单通道
    cv::Mat edge(edges.h, edges.w, CV_8UC1);
    for (int y = 0; y < edges.h; ++y)
        for (int x = 0; x < edges.w; ++x)
            edge.at<uchar>(y, x) =
                static_cast<uchar>(clamp255(edges.data[y][x * edges.ch]));

    // RETR_LIST：保留所有轮廓，包括闭合圆环的内边界（洞）。
    // 内外两条轮廓会拟合成同一个圆，后面按中心距离去重。
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edge, contours, cv::RETR_LIST, cv::CHAIN_APPROX_NONE);

    std::vector<Candidate> kept;
    kept.reserve(contours.size());

    for (const auto& contour : contours) {
        if (contour.size() < 5) continue;   // fitEllipse 至少需要 5 个点

        const double length = cv::arcLength(contour, true);
        if (length < 20.0 || length > 600.0) continue;

        const cv::RotatedRect ell = cv::fitEllipse(contour);
        const double major = std::max(ell.size.width, ell.size.height);
        const double minor = std::min(ell.size.width, ell.size.height);
        if (major < params.min_major || major > params.max_major) continue;
        if (major <= 0.0 || minor / major < params.min_axis_ratio) continue;

        const double residual = fit_residual(contour, ell);
        if (residual > params.max_residual) continue;

        kept.push_back({ell, residual, static_cast<int>(contour.size())});
    }

    // 同一圆环的内外轮廓中心几乎重合：保留点更多（外轮廓）或残差更小的一条
    std::vector<Candidate> deduped;
    deduped.reserve(kept.size());
    for (const Candidate& c : kept) {
        bool merged = false;
        for (Candidate& d : deduped) {
            const double dist = cv::norm(c.ellipse.center - d.ellipse.center);
            if (dist >= params.dedupe_distance) continue;
            if (c.points > d.points ||
                (c.points == d.points && c.residual < d.residual))
                d = c;
            merged = true;
            break;
        }
        if (!merged) deduped.push_back(c);
    }

    result.reserve(deduped.size());
    for (size_t i = 0; i < deduped.size(); ++i) {
        const cv::RotatedRect& ell = deduped[i].ellipse;
        const double major = std::max(ell.size.width, ell.size.height);
        const double minor = std::min(ell.size.width, ell.size.height);

        // RotatedRect 的 angle 对应 width 轴；长轴是 height 时补 90°
        double angle = ell.angle;
        if (ell.size.height > ell.size.width) angle += 90.0;
        angle = std::fmod(angle, 180.0);

        Component c;
        c.label = static_cast<int>(i) + 1;
        c.cx = ell.center.x;
        c.cy = ell.center.y;
        c.major = major;
        c.minor = minor;
        c.angle = angle;
        c.residual = deduped[i].residual;
        c.area = static_cast<int>(std::lround(CV_PI * major * minor / 4.0));

        const cv::Rect box = ell.boundingRect();
        c.min_x = std::clamp(box.x, 0, edges.w - 1);
        c.min_y = std::clamp(box.y, 0, edges.h - 1);
        c.max_x = std::clamp(box.x + box.width - 1, 0, edges.w - 1);
        c.max_y = std::clamp(box.y + box.height - 1, 0, edges.h - 1);
        if (c.max_x < c.min_x) c.max_x = c.min_x;
        if (c.max_y < c.min_y) c.max_y = c.min_y;

        result.push_back(c);
    }
    return result;
}

}  // namespace calib
