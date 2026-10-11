#include "includes/image_io.hpp"
#include "includes/median_blur.hpp"
#include "includes/pipeline.hpp"
#include "includes/draw_center.hpp"
#include "includes/export_coords.hpp"
#include "includes/kernels.hpp"
#include <iostream>
#include <string>

// 用法: final <输入图> <输出图> <方法>
//   median   中值滤波
//   pipeline 中值滤波 + Otsu 反向二值化 + BFS 连通域标记
int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "用法: " << argv[0] << " <输入图> <输出图> <median|pipeline>\n";
        return 1;
    }

    Image img = read_image(argv[1]);
    std::string method = argv[3];

    const std::string out = argv[2];

    if (method == "pipeline") {
        PipelineResult res = pipeline(img, median_default_window);
        std::cout << "连通域: " << res.labels.components.size()
                  << "  圆形候选: " << res.candidates.size() << "\n";
        std::cout << "大圆: " << res.count.large
                  << "  中圆: " << res.count.medium
                  << "  小圆: " << res.count.small << "\n";
        save_image(out, res.binary);

        // 标注图和坐标文件路径由输出参数派生
        const std::string stem = out.substr(0, out.find_last_of('.'));
        const std::string centers = stem + "_centers.png";
        const std::string coords = stem + "_coords.csv";

        if (!draw_center(img, res.candidates, centers, 18, 2))
            std::cerr << "绘制中心失败\n";
        if (!export_coords(res.candidates, coords))
            std::cerr << "导出坐标失败\n";
    } else {
        save_image(out, median_blur(img, median_default_window));
    }

    std::cout << "已保存: " << out << "\n";
    return 0;
}
