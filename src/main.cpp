#include "calib/draw_center.hpp"
#include "calib/export_coords.hpp"
#include "calib/image_io.hpp"
#include "calib/kernels.hpp"
#include "calib/median_blur.hpp"
#include "calib/pipeline.hpp"

#include <filesystem>
#include <iostream>
#include <string>

// 用法: pipeline <输入图> [输出根目录=result] [median|pipeline，默认 pipeline]
//   median   中值滤波
//   pipeline 中值滤波 + Otsu 反向二值化 + BFS 连通域标记
int main(int argc, char** argv) {
    if (argc < 2 || argc > 4) {
        std::cerr << "用法: " << argv[0] << " <输入图> [输出根目录=result] [median|pipeline，默认 pipeline]\n";
        return 1;
    }

    const std::string in_path = argv[1];
    const std::string method = argc >= 4 ? argv[3] : "pipeline";
    if (method != "median" && method != "pipeline") {
        std::cerr << "未知方法: " << method << "（可选 median|pipeline）\n";
        return 1;
    }
    namespace fs = std::filesystem;

    const fs::path output_root = argc >= 3 ? argv[2] : "result";
    const std::string extension = output_root.extension().string();
    if (extension == ".png" || extension == ".bmp" || extension == ".jpg" || extension == ".jpeg") {
        std::cerr << "第二个参数应为输出根目录（例如 result），不再接受输出图片路径\n";
        return 1;
    }
    const fs::path out_dir = output_root / fs::path(in_path).stem();

    calib::Image img = calib::read_image(in_path);
    if (img.data.empty()) {
        std::cerr << "读取失败: " << in_path << "\n";
        return 1;
    }

    std::error_code error;
    fs::create_directories(out_dir, error);
    if (error) {
        std::cerr << "无法创建输出目录: " << out_dir << ": " << error.message() << '\n';
        return 1;
    }

    if (method == "pipeline") {
        const calib::PipelineResult res = calib::pipeline(img, calib::median_default_window);
        std::cout << "连通域: " << res.labels.components.size()
                  << "  圆形候选: " << res.candidates.size() << "\n";
        std::cout << "大圆: " << res.count.large
                  << "  中圆: " << res.count.medium
                  << "  小圆: " << res.count.small << "\n";
        if (res.numbering.success) {
            std::cout << "网格编号成功: " << res.numbering.rows << " 行, "
                      << res.numbering.cols << " 列\n";
        } else {
            std::cout << "网格编号失败: " << res.numbering.message << '\n';
        }
        const bool binary_saved = calib::save_image((out_dir / "binary.png").string(), res.binary);
        const bool annotated_saved = calib::draw_center(
            img, res.candidates, (out_dir / "annotated.png").string(), 18, 2);
        const bool coords_saved = calib::export_coords(
            res.candidates, (out_dir / "coordinates.csv").string());
        if (!binary_saved || !annotated_saved || !coords_saved) {
            std::cerr << "输出失败: binary=" << binary_saved
                      << ", annotated=" << annotated_saved << ", coordinates=" << coords_saved << '\n';
            return 1;
        }
    } else if (method == "median") {
        if (!calib::save_image((out_dir / "median.png").string(),
                               calib::median_blur(img, calib::median_default_window))) {
            std::cerr << "保存中值滤波图失败\n";
            return 1;
        }
    }

    std::cout << "结果目录: " << out_dir << "\n";
    return 0;
}
