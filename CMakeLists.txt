cmake_minimum_required(VERSION 3.16)

project(calibration
        VERSION 0.1.0
        DESCRIPTION "工业摄影测量：圆形标记点识别与中心提取"
        LANGUAGES CXX)

message(STATUS "项目根目录: ${PROJECT_SOURCE_DIR}")

# 未指定构建类型时默认 Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "构建类型" FORCE)
endif()
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# OpenCV（仅 draw_center 用于绘制标注图）
find_package(OpenCV 4 REQUIRED)
message(STATUS "OpenCV 版本: ${OpenCV_VERSION}")

# 添加可执行文件
add_executable(pipeline
    src/main.cpp
    src/kernels.cpp
    src/image_io.cpp
    src/median_blur.cpp
    src/threshold.cpp
    src/pipeline.cpp
    src/bfs_label.cpp
    src/pick.cpp
    src/classify.cpp
    src/draw_center.cpp
    src/export_coords.cpp
)

# target 专属设置
target_compile_features(pipeline PRIVATE cxx_std_17)

target_include_directories(pipeline PRIVATE
    ${PROJECT_SOURCE_DIR}/include       # 本项目头文件，形如 #include "calib/xxx.hpp"
)

# 第三方头文件标记为 SYSTEM，避免其内部警告污染本项目编译输出
target_include_directories(pipeline SYSTEM PRIVATE
    ${PROJECT_SOURCE_DIR}/third_party   # 形如 #include "stb/stb_image.h"
)

target_link_libraries(pipeline PRIVATE ${OpenCV_LIBS})
target_include_directories(pipeline PRIVATE ${OpenCV_INCLUDE_DIRS})

target_compile_options(pipeline PRIVATE
    -Wall
    -Wextra
)

set_target_properties(pipeline PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)
