// 在原图上标注连通域中心（红色十字）

#include "includes/draw_center.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <vector>

bool draw_center(const Image& raw,
                 const std::vector<Component>& comps,
                 const std::string& out_path,
                 int arm,
                 int thickness) {
    if (raw.data.empty() || raw.w <= 0 || raw.h <= 0) return false;

    // 把内部 Image 数据包成 cv::Mat。Image 为逐像素多通道排列，
    // 这里复制到一块连续缓冲，避免对非连续数据做原地绘制。
    const int ch = raw.ch;
    cv::Mat canvas(raw.h, raw.w, CV_8UC(ch));
    for (int y = 0; y < raw.h; ++y) {
        for (int x = 0; x < raw.w; ++x) {
            uchar* p = canvas.ptr<uchar>(y) + static_cast<size_t>(x) * ch;
            for (int c = 0; c < ch; ++c)
                p[c] = static_cast<uchar>(clamp255(raw.data[y][x * ch + c]));
        }
    }

    // 统一转成 3 通道 BGR 再绘制：
    // 若保留 4 通道，cv::line 的 Scalar 第 4 分量会被当成 alpha，
    // 只给 3 个分量会把 alpha 写为 0，红线就变成透明/灰白。
    if (ch == 1) {
        cv::cvtColor(canvas, canvas, cv::COLOR_GRAY2BGR);
    } else if (ch == 4) {
        cv::cvtColor(canvas, canvas, cv::COLOR_BGRA2BGR);
    } else if (ch == 2) {
        cv::cvtColor(canvas, canvas, cv::COLOR_GRAY2BGR);   // 灰度+alpha，保留灰度
    }

    const cv::Scalar red(0, 0, 255);      // BGR 顺序，中圆：纯红
    const cv::Scalar green(0, 255, 0);    // 大圆：纯绿
    const cv::Scalar orange(0, 165, 255); // 小圆：橙色

    for (const Component& c : comps) {
        const int cx = static_cast<int>(c.cx + 0.5);
        const int cy = static_cast<int>(c.cy + 0.5);

        // 按类别选颜色：大圆绿、小圆黄、其余红
        cv::Scalar color = red;
        if (c.type == CircleType::Large) color = green;
        else if (c.type == CircleType::Small) color = orange;

        const cv::Point h1(cx - arm, cy), h2(cx + arm, cy);
        const cv::Point v1(cx, cy - arm), v2(cx, cy + arm);

        // LINE_8 保证是纯色；LINE_AA 抗锯齿会与背景混色，显得不纯
        cv::line(canvas, h1, h2, color, thickness, cv::LINE_8);
        cv::line(canvas, v1, v2, color, thickness, cv::LINE_8);
    }

    return cv::imwrite(out_path, canvas);
}
