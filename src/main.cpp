#include "calib/draw_center.hpp"
#include "calib/export_coords.hpp"
#include "calib/image_io.hpp"
#include "calib/kernels.hpp"
#include "calib/median_blur.hpp"
#include "calib/pipeline.hpp"

#include <iostream>
#include <string>

// 用法: pipeline <输入图> <输出图> <median|pipeline>
//   median   中值滤波
//   pipeline 中值滤波 + Otsu 反向二值化 + BFS 连通域标记
int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "用法: " << argv[0] << " <输入图> <输出图> <median|pipeline>\n";
        return 1;
    }

    const std::string in_path = argv[1];
    const std::string out = argv[2];
    const std::string method = argv[3];

    calib::Image img = calib::read_image(in_path);
    if (img.data.empty()) {
        std::cerr << "读取失败: " << in_path << "\n";
        return 1;
    }

    if (method == "pipeline") {
        const calib::PipelineResult res = calib::pipeline(img, calib::median_default_window);
        std::cout << "连通域: " << res.labels.components.size()
                  << "  圆形候选: " << res.candidates.size() << "\n";
        std::cout << "大圆: " << res.count.large
                  << "  中圆: " << res.count.medium
                  << "  小圆: " << res.count.small << "\n";
        calib::save_image(out, res.binary);

        // 标注图和坐标文件路径由输出参数派生
        const std::string stem = out.substr(0, out.find_last_of('.'));
        const std::string centers = stem + "_centers.png";
        const std::string coords = stem + "_coords.csv";

        if (!calib::draw_center(img, res.candidates, centers, 18, 2))
            std::cerr << "绘制中心失败\n";
        if (!calib::export_coords(res.candidates, coords))
            std::cerr << "导出坐标失败\n";
    } else if (method == "median") {
        calib::save_image(out, calib::median_blur(img, calib::median_default_window));
    } else {
        std::cerr << "未知方法: " << method << "（可选 median|pipeline）\n";
        return 1;
    }

    std::cout << "已保存: " << out << "\n";
    return 0;
}
