//stb_image的实现在.h文件中需要先定义才能展开编译
//全项目唯一定义图片读写实现的地方，避免链接失败
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "includes/image_io.hpp"
#include "includes/stb_image.h"
#include "includes/stb_image_write.h"
#include <iostream>

Image read_image(const std::string& path) {
    Image img;

    // 第 5 个参数传 0 = 保留原图通道数
    // buf：[ R0 G0 B0 R1 G1 B1 R2 G2 B2 ... ]
    auto* buf = stbi_load(path.c_str(), &img.w, &img.h, &img.ch, 0);

    //空图片主动抛出异常
    if (!buf) {
        std::cerr << "读取失败 " << path << ": "
                  << stbi_failure_reason() << "\n";
        return img;   // 所有字段为默认值，data 为空
    }

    //逐像素把 stb 的连续缓冲拷贝进 Matrix：
    img.data = Matrix(img.h, std::vector<int>(img.w * img.ch, 0));
    for (int y = 0; y < img.h; ++y)
        for (int x = 0; x < img.w; ++x)
            for (int c = 0; c < img.ch; ++c)
                // buf：[ R00 G00 B00 R01 G01 B01 R02 G02 B02 ... ]
                // img.data：[ [R00 G00 B00 R01 G01 B01 ...] , [R10 G10 B10 R11 G11 B11 ...] , ... ]
                img.data[y][x * img.ch + c] = buf[(y * img.w + x) * img.ch + c];  // 0 ~ 255


    stbi_image_free(buf);   // 释放 stb_load 占用的内存
    return img;
}

// 保存图片为stbi_image的一维数组
bool save_image(const std::string& path, const Image& img) {
    const int w = img.w;
    const int h = img.h;
    const int ch = img.ch;   // 灰度=1，彩色=3

    //定义一维缓冲数组buf
    std::vector<unsigned char> buf(w * h * ch);
    // 先把 Matrix(int) 展成连续 unsigned char 缓冲，并 clamp 到 0~255
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            for (int c = 0; c < ch; ++c) {
                int v = img.data[y][x * ch + c];
                v = clamp255(v);   // clamp(钳位)
                // 展平为buf(stbi可读的一维缓冲数组)
                buf[(y * w + x) * ch + c] = static_cast<unsigned char>(v);
            }
        }
    }

    // stbi_write_png 返回非 0 表示成功（1），失败时返回 0
    return stbi_write_png(path.c_str(), w, h, ch, buf.data(), w * ch) != 0;
}
