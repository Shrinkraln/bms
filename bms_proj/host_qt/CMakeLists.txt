# BMS-5S 化成上位机 (Qt, 通过 QtSerialBus 的 peakcan 插件接 PEAK PCAN)
#
# 依赖:
#   - Qt 5.15+ 或 Qt 6, 含 Widgets + SerialBus 模块
#   - PEAK 驱动已安装 (提供 PCANBasic.dll, peakcan 插件运行时加载它)
#
# 构建 (命令行示例):
#   cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.7.2/mingw_64
#   cmake --build build
# 或直接用 Qt Creator 打开本 CMakeLists.txt。

cmake_minimum_required(VERSION 3.16)
project(bms_host LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets SerialBus)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets SerialBus)

add_executable(bms_host
    main.cpp
    mainwindow.h
    mainwindow.cpp
    plotwidget.h
    plotwidget.cpp
    protocol.h
)

target_link_libraries(bms_host PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::SerialBus
)
