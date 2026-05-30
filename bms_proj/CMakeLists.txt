cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME bms_proj)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM, C and C++ (LVGL 子项目 project() 要 CXX)
enable_language(C CXX ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# ===== LVGL 9.x =====
# 关掉 thorvg / demos / examples (省 flash + 编译时间)
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS           ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_EXAMPLES        ON CACHE BOOL "" FORCE)
add_subdirectory(Middlewares/Third_Party/lvgl EXCLUDE_FROM_ALL)
# 让 LVGL 源码看到我们的 lv_conf.h ("#include \"lv_conf.h\"" 默认行为)
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/BSP/Inc)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
    
)


# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    BSP/Src/bsp.c
    BSP/Src/i2c_bus.c
    BSP/Src/bq76920.c
    BSP/Src/periph_tests.c
    BSP/Src/dac8552.c
    BSP/Src/lcd_st7796.c
    BSP/Src/can_test.c
    BSP/Src/can_report.c
    BSP/Src/can_cmd.c
    BSP/Src/bq76_alert.c
    BSP/Src/watchdog.c
    BSP/Src/formation.c
    BSP/Src/ft6336u.c
    BSP/Src/lvgl_port.c
    BSP/Src/bms_ui.c
    BSP/Src/my_main.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    BSP/Inc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    lvgl

    # Add user defined libraries
)
