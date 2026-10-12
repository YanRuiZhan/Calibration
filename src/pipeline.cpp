// 图像预处理流水线：串联多步处理，供 main 调用

#include "calib/pipeline.hpp"
#include "calib/median_blur.hpp"
#include "calib/threshold.hpp"
#include "calib/bfs_label.hpp"
#include "calib/pick.hpp"
#include "calib/classify.hpp"

#include <vector>

namespace calib {

PipelineResult pipeline(const Image& raw, int win) {
    PipelineResult result;

    // 第一步：中值滤波去噪
    // 窗口非法（非正/偶数）时视为不需要滤波，直接用原图
    Image denoised = raw;
    if (win > 0 && win % 2 == 1) {
        denoised = median_blur(raw, win);
    }

    // 第二步：Otsu 反向二值化，让黑色圆点成为前景
    Image binary = otsu_threshold_inv(denoised);

    // 第三步：BFS 连通域标记
    // bfs_label 只接受单通道矩阵，这里取二值图的第 0 通道
    Matrix single(binary.h, std::vector<int>(binary.w, 0));
    for (int y = 0; y < binary.h; ++y)
        for (int x = 0; x < binary.w; ++x)
            single[y][x] = binary.data[y][x * binary.ch];

    result.binary = std::move(binary);
    result.labels = bfs_label(single);

    // 第四步：按形状挑出圆形靶点候选
    result.candidates = pick_circular(result.labels.components, PickParams{});

    // 第五步：按面积区分大、中、小圆并统计
    result.count = classify_circles(result.candidates);
    return result;
}

}  // namespace calib
