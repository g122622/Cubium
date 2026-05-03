cmake_minimum_required(VERSION 3.26)

# vcpkg 集成（必须在 project() 之前）
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
elseif(EXISTS "D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake" AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
elseif(EXISTS "$ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake" AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

# 项目定义
project(MinecraftReborn
    VERSION 0.1.0
    DESCRIPTION "Modern Minecraft Clone with Client-Server Architecture"
    LANGUAGES CXX C
)

# C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
option(MC_BUILD_CLIENT "Build client application" ON)
option(MC_BUILD_SERVER "Build server application" ON)
option(MC_BUILD_TESTS "Build unit tests" ON)
option(MC_ENABLE_SANITIZERS "Enable sanitizers for debug builds" OFF)
option(MC_ENABLE_VULKAN_VALIDATION "Enable Vulkan validation layers" OFF)
option(MC_ENABLE_TRACING "Enable Perfetto tracing (default OFF)" OFF)
option(MC_ENABLE_CCACHE "Enable ccache for faster rebuilds" ON)

# ccache 配置（仅 Unix 平台：Linux/macOS）
if(MC_ENABLE_CCACHE AND UNIX)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_CUDA_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    else()
        message(STATUS "ccache not found, building without ccache")
    endif()
endif()

if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# CMake模块
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# 编译配置
include(CompilerWarnings)

# 查找依赖包
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Vulkan REQUIRED)

# 全局编译选项
if(MSVC)
    add_compile_options(/MP /utf-8)
    # 强制串行化 PDB 写入，解决多编译器实例同时写入同一 PDB 文件的问题
    add_compile_options(/FS)
    # 架构专用优化：启用 AVX2 指令集，加速向量运算
    add_compile_options(/arch:AVX2)
    # 启用快速浮点数
    add_compile_options(/fp:fast)
    # 禁用浮点异常
    add_compile_options(/fp:except-)
    # 使用编译器内置的、更高效的函数版本替换某些标准库函数或操作
    add_compile_options(/Oi)
    # 链接器选项，移除所有未引用的函数和数据
    add_link_options(/OPT:REF)
    # 链接器选项，合并相同的常量折叠（COMDAT folding），进一步减小代码体积
    add_link_options(/OPT:ICF)
    # 全程序优化
    add_link_options(/LTCG)
    # 自动向量化 会与 GL 冲突
    # add_compile_options(/Qvec-report:2)
else()
    # O3
    add_compile_options(-O3)
    add_compile_options(-fcolor-diagnostics)
    # 架构专用优化：针对本地 CPU 生成优化指令
    add_compile_options(-march=native -mtune=native)
    # 启用快速浮点数
    add_compile_options(-ffast-math)
    # 禁用浮点异常
    add_compile_options(-fno-trapping-math)
    # 链接器选项，移除所有未引用的函数和数据
    add_link_options(-Wl)

endif()

if(WIN32)
    # 统一 Windows 目标版本，避免 Asio 反复发出 _WIN32_WINNT 警告。
    add_compile_definitions(_WIN32_WINNT=0x0A00 WINVER=0x0A00)
endif()

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 版本头文件
configure_file(
    "${CMAKE_SOURCE_DIR}/include/minecraft-reborn/version.h.in"
    "${CMAKE_SOURCE_DIR}/include/minecraft-reborn/version.h"
    @ONLY
)

# 子项目
add_subdirectory(src/common)

if(MC_BUILD_SERVER)
    add_subdirectory(src/server)
endif()

if(MC_BUILD_CLIENT)
    add_subdirectory(src/client)
    add_subdirectory(shaders)
endif()

if(MC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 安装配置
include(GNUInstallDirs)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# 打印配置摘要
message(STATUS "")
message(STATUS "=== Minecraft Reborn Configuration ===")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  Build Type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Client:   ${MC_BUILD_CLIENT}")
message(STATUS "  Build Server:   ${MC_BUILD_SERVER}")
message(STATUS "  Build Tests:    ${MC_BUILD_TESTS}")
message(STATUS "  Sanitizers:     ${MC_ENABLE_SANITIZERS}")
message(STATUS "  Vulkan Val:     ${MC_ENABLE_VULKAN_VALIDATION}")
message(STATUS "  Tracing:        ${MC_ENABLE_TRACING}")
if(MC_ENABLE_CCACHE AND UNIX AND CCACHE_PROGRAM)
    message(STATUS "  ccache:         Enabled (${CCACHE_PROGRAM})")
elseif(MC_ENABLE_CCACHE AND UNIX)
    message(STATUS "  ccache:         Not found")
elseif(MC_ENABLE_CCACHE)
    message(STATUS "  ccache:         Skipped (Unix only)")
else()
    message(STATUS "  ccache:         Disabled")
endif()
message(STATUS "======================================")
message(STATUS "")
