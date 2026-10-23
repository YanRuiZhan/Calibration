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

# OpenCV（Canny、轮廓/椭圆拟合、标注图绘制）
# 整个导入省去用到新模块时再改这里的麻烦
find_package(OpenCV 4 REQUIRED)
message(STATUS "OpenCV 版本: ${OpenCV_VERSION}")

# 添加可执行文件（src 按职责分层）
add_executable(pipeline
    src/app/main.cpp
    src/app/pipeline.cpp
    src/core/kernels.cpp
    src/core/convolution.cpp
    src/core/image_io.cpp
    src/core/gray.cpp
    src/detect/canny.cpp
    src/detect/ellipse.cpp
    src/numbering/classify.cpp
    src/numbering/grid_numbering.cpp
    src/output/draw_center.cpp
    src/output/export_coords.cpp
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
# 头文件路径不需要显式添加：OpenCV 的导入目标自带 INTERFACE_INCLUDE_DIRECTORIES，
# 会以 -isystem /usr/include/opencv4 注入（这也正是 <opencv2/core.hpp> 能直接写的原因）

target_compile_options(pipeline PRIVATE
    -std=c++17
    -Wall
    -Wextra
)

set_target_properties(pipeline PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)

option(BUILD_GUI "构建 Qt 图片标点编号界面" ON)
if(BUILD_GUI)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

    add_executable(calibration_gui src/app/gui_main.cpp)
    target_compile_features(calibration_gui PRIVATE cxx_std_17)
    target_compile_options(calibration_gui PRIVATE -Wall -Wextra)
    target_link_libraries(calibration_gui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
    set_target_properties(calibration_gui PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
    )
    # GUI 异步调用同目录的命令行程序，复用原有算法与输出格式。
    add_dependencies(calibration_gui pipeline)
endif()
