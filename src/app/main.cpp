#include "calib/draw_center.hpp"
#include "calib/export_coords.hpp"
#include "calib/image_io.hpp"
#include "calib/pipeline.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

//   pipeline <输入图> [输出根目录=result]
//   原图 -> 改进 Canny -> 全图 findContours/fitEllipse -> 分类 -> 主方向编号
int main(int argc, char **argv) {
  if (argc < 2 || argc > 3) {
    std::cerr << "用法: " << argv[0]
              << " <输入图> [输出根目录=result]\n";
    return 1;
  }

  const std::string in_path = argv[1];
  namespace fs = std::filesystem;

  const fs::path output_root = argc >= 3 ? argv[2] : "result";
  // 第二个参数必须是目录，不能是输出图片路径
  const std::string extension = output_root.extension().string();
  if (extension == ".png" || extension == ".bmp" || extension == ".jpg" ||
      extension == ".jpeg") {
    std::cerr
        << "第二个参数应为输出根目录（例如 result），不再接受输出图片路径\n";
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
    std::cerr << "无法创建输出目录: " << out_dir << ": " << error.message()
              << '\n';
    return 1;
  }

  calib::PipelineResult res;
  try {
    res = calib::pipeline(img);
  } catch (const std::exception &e) {
    std::cerr << "处理失败: " << e.what() << '\n';
    return 1;
  }

  std::cout << "改进 Canny  圆形候选: " << res.candidates.size() << "\n";
  std::cout << "大圆: " << res.count.large << "  中圆: " << res.count.medium
            << "  小圆: " << res.count.small << "\n";
  if (res.numbering.success) {
    std::cout << res.numbering.message << ": " << res.numbering.rows << " 行, "
              << res.numbering.cols << " 列\n";
  } else {
    std::cout << "网格编号失败: " << res.numbering.message << '\n';
  }

  const bool binary_saved =
      calib::save_image((out_dir / "binary.png").string(), res.binary);
  const bool annotated_saved =
      calib::draw_center(img, res.candidates,
                         (out_dir / "annotated.png").string(), 18, 2);
  const bool coords_saved = calib::export_coords(
      res.candidates, (out_dir / "coordinates.csv").string());
  if (!binary_saved || !annotated_saved || !coords_saved ||
        !initial_saved || !centers_saved || !classified_saved || !direction_saved) {
    std::cerr << "输出失败: binary=" << binary_saved
              << ", annotated=" << annotated_saved
              << ", coordinates=" << coords_saved
                            << ", initial=" << initial_saved << ", centers=" << centers_saved
                            << ", classified=" << classified_saved << ", direction=" << direction_saved << '\n';
    return 1;
  }

  std::cout << "结果目录: " << out_dir << "\n";
  return 0;
}
