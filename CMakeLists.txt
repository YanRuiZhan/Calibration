cmake_minimum_required(VERSION 3.16)

project(assignment_ws
        VERSION 0.1.0
        LANGUAGES CXX)

message(STATUS "项目根目录: ${PROJECT_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 未指定构建类型时默认 Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "构建类型" FORCE)
endif()
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")

# OpenCV
find_package(OpenCV 4 REQUIRED)
message(STATUS "OpenCV 版本: ${OpenCV_VERSION}")
message(STATUS "OpenCV 头文件: ${OpenCV_INCLUDE_DIRS}")

# 添加可执行文件
add_executable(pipeline
    src/main.cpp
    src/convolution.cpp
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
target_include_directories(pipeline PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/includes
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(pipeline PRIVATE ${OpenCV_LIBS})


target_compile_options(pipeline PRIVATE
    -Wall
    -Wextra
)

set_target_properties(pipeline PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)
