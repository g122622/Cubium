cmake_minimum_required(VERSION 3.26)

# vcpkg 集成（必须在 project() 之前）
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
elseif(EXISTS "E:/vcpkg/scripts/buildsystems/vcpkg.cmake" AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "E:/vcpkg/scripts/buildsystems/vcpkg.cmake"
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
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
option(MC_BUILD_CLIENT "Build client application" ON)
option(MC_BUILD_SERVER "Build server application" ON)
option(MC_BUILD_TESTS "Build unit tests" ON)
option(MC_BUILD_BENCHMARKS "Build benchmark application" ON)
option(MC_ENABLE_SANITIZERS "Enable sanitizers for debug builds" OFF)
option(MC_ENABLE_VULKAN_VALIDATION "Enable Vulkan validation layers" OFF)
option(MC_ENABLE_TRACING "Enable Perfetto tracing (default OFF)" ON)
option(MC_ENABLE_TRACY "Enable Tracy profiling (default OFF)" ON)
option(MC_ENABLE_MEMORY "Enable memory allocation tracking via Tracy (default OFF)" ON)
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

# ============================================================
# 构建信息收集
# ============================================================

# 查找 Git
find_package(Git QUIET)

# 获取 Git 提交哈希
if(GIT_FOUND)
    # 短哈希 (7位)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short=7 HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE MC_GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE GIT_HASH_RESULT
    )

    # 完整哈希
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE MC_GIT_COMMIT_HASH_FULL
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE GIT_HASH_FULL_RESULT
    )

    # 分支名称
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE MC_GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE GIT_BRANCH_RESULT
    )

    # 检查是否有未提交的更改
    execute_process(
        COMMAND ${GIT_EXECUTABLE} status --porcelain
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE MC_GIT_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # 如果有未提交的更改，标记为 dirty
    if(NOT "${MC_GIT_STATUS}" STREQUAL "")
        set(MC_GIT_DIRTY true)
    else()
        set(MC_GIT_DIRTY false)
    endif()

    # 处理获取失败的情况
    if(NOT GIT_HASH_RESULT EQUAL 0)
        set(MC_GIT_COMMIT_HASH "unknown")
    endif()
    if(NOT GIT_HASH_FULL_RESULT EQUAL 0)
        set(MC_GIT_COMMIT_HASH_FULL "unknown")
    endif()
    if(NOT GIT_BRANCH_RESULT EQUAL 0)
        set(MC_GIT_BRANCH "unknown")
    endif()
else()
    set(MC_GIT_COMMIT_HASH "unknown")
    set(MC_GIT_COMMIT_HASH_FULL "unknown")
    set(MC_GIT_BRANCH "unknown")
    set(MC_GIT_DIRTY false)
endif()

# 构建时间 (UTC)
string(TIMESTAMP MC_BUILD_TIME UTC)

# 构建类型
if(CMAKE_BUILD_TYPE STREQUAL "")
    set(MC_BUILD_TYPE "Release")
else()
    set(MC_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
endif()

# 编译器信息
set(MC_COMPILER_ID "${CMAKE_CXX_COMPILER_ID}")
set(MC_COMPILER_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
set(MC_COMPILER_STRING "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# 平台信息
if(WIN32)
    set(MC_BUILD_PLATFORM "Windows")
elseif(APPLE)
    set(MC_BUILD_PLATFORM "macOS")
elseif(UNIX)
    set(MC_BUILD_PLATFORM "Linux")
else()
    set(MC_BUILD_PLATFORM "Unknown")
endif()

# 架构信息
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(MC_BUILD_ARCH "x64")
else()
    set(MC_BUILD_ARCH "x86")
endif()

# 查找依赖包
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(simdjson CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Vulkan REQUIRED)
find_package(RocksDB CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(leveldb CONFIG REQUIRED)
find_package(gflags CONFIG REQUIRED)
find_package(asmjit CONFIG REQUIRED)

# ============================================================
# QuickJS-NG - JavaScript 引擎，用于基岩版Addon脚本系统
# ============================================================
# 使用quickjs-ng（支持Windows），而非原始quickjs
include(FetchContent)
FetchContent_Declare(
    quickjs-ng
    GIT_REPOSITORY https://github.com/quickjs-ng/quickjs.git
    GIT_TAG        v0.15.0
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(QJS_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(QJS_BUILD_CLI_STATIC OFF CACHE BOOL "" FORCE)
set(QJS_BUILD_WERROR OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(quickjs-ng)

# 修复quickjs-ng C库在Clang+MSVC环境下找不到vcruntime.h的问题
# 原因：configure.sh设置了clang++的MSVC include路径，但C编译器clang.exe缺少这些路径
if(WIN32 AND CMAKE_C_COMPILER_ID MATCHES "Clang")
    # 获取MSVC include目录
    if(DEFINED ENV{VCToolsInstallDir})
        set(_MSVC_INC_DIR "$ENV{VCToolsInstallDir}/include")
        if(TARGET qjs)
            target_include_directories(qjs PRIVATE ${_MSVC_INC_DIR})
        endif()
        if(TARGET qjs-libc)
            target_include_directories(qjs-libc PRIVATE ${_MSVC_INC_DIR})
        endif()
    endif()
endif()

# 全局编译选项
add_compile_options(-fcolor-diagnostics)

if(MC_ENABLE_SANITIZERS)
    # Sanitizer 构建：禁用优化和可能干扰 sanitizer 的选项
    # CMAKE_CXX_FLAGS 中由调用者注入具体的 -fsanitize= 选项
    add_compile_options(-O1 -fno-omit-frame-pointer -fno-common)
    add_compile_options(-fno-fast-math) # sanitizer 需要精确浮点语义
    message(STATUS "Sanitizer build: optimizations reduced, stack protection enabled")
else()
    add_compile_options(-O2) # 不开o3，o3对缓存不友好、增加编译时间且可能存在bug
    add_compile_options(-march=native -mtune=native) # 架构专用优化：针对本地 CPU 生成优化指令
    add_compile_options(-ffast-math -fno-finite-math-only) # 启用快速浮点数，但保留 NaN/Inf 语义以允许 isfinite 检查
    add_compile_options(-fno-trapping-math) # 禁用浮点异常
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
        add_compile_options(-mavx2) # 启用 AVX2 指令集（仅 x86_64，arm64 不支持）
    endif()
    add_compile_options(-fno-stack-protector -fno-stack-check) # 关闭栈保护，减少性能开销（注意安全风险）
    # 链接时优化。只在release构建启用
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-flto=thin) # 启用链接时优化（LTO），使用 thin 模式以减少编译时间
        if(WIN32 OR LINUX)
            add_link_options(-flto=thin -fuse-ld=lld)
        else()
            add_link_options(-flto=thin)
        endif()
        if(WIN32 OR LINUX)
            add_link_options(-Wl,-gc-sections) # 链接器选项，移除所有未引用的函数和数据（GNU ld/ld.lld）
        endif()
    endif()
endif()

if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Configuring Windows build with Clang: ${CMAKE_CXX_COMPILER}")
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

# ============================================================
# OffsetAllocator - GPU buffer 子分配器（vendored submodule, third_party/OffsetAllocator）
# O(1) TLSF 风格 1D offset 分配器，用于 StagingPool 与 mega-buffer 子分配。
# 上游 CMakeLists.txt 依赖项目私有 setup_target_libs()，无法直接 add_subdirectory，
# 故直接编 offsetAllocator.cpp 进独立 STATIC lib，暴露头文件路径。
# ============================================================
add_library(mc_offset_allocator STATIC
    ${CMAKE_SOURCE_DIR}/third_party/OffsetAllocator/offsetAllocator.cpp)
target_include_directories(mc_offset_allocator PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/OffsetAllocator)
target_compile_features(mc_offset_allocator PUBLIC cxx_std_20)
# OffsetAllocator 为 prototype 三方库，单独关闭警告以免污染构建输出
if(MSVC)
    target_compile_options(mc_offset_allocator PRIVATE /W0 /permissive-)
else()
    target_compile_options(mc_offset_allocator PRIVATE -w)
endif()

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

if(MC_BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()

# 安装配置
include(GNUInstallDirs)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# 打印配置摘要
message(STATUS "")
message(STATUS "=== Cubium Configuration ===")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  Build Type:     ${MC_BUILD_TYPE}")
message(STATUS "  Platform:       ${MC_BUILD_PLATFORM} ${MC_BUILD_ARCH}")
message(STATUS "  C Compiler:     ${CMAKE_C_COMPILER_ID} (${CMAKE_C_COMPILER})")
message(STATUS "  CXX Compiler:   ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER})")
message(STATUS "  Git Branch:     ${MC_GIT_BRANCH}")
message(STATUS "  Git Commit:     ${MC_GIT_COMMIT_HASH}")
if(MC_GIT_DIRTY)
    message(STATUS "  Git Status:     Dirty (uncommitted changes)")
else()
    message(STATUS "  Git Status:     Clean")
endif()
message(STATUS "  Build Time:     ${MC_BUILD_TIME}")
message(STATUS "  Build Client:   ${MC_BUILD_CLIENT}")
message(STATUS "  Build Bench:    ${MC_BUILD_BENCHMARKS}")
message(STATUS "  Build Server:   ${MC_BUILD_SERVER}")
message(STATUS "  Build Tests:    ${MC_BUILD_TESTS}")
message(STATUS "  Sanitizers:     ${MC_ENABLE_SANITIZERS}")
message(STATUS "  Vulkan Val:     ${MC_ENABLE_VULKAN_VALIDATION}")
message(STATUS "  Tracing:        ${MC_ENABLE_TRACING}")
message(STATUS "  Tracy:          ${MC_ENABLE_TRACY}")
message(STATUS "  Memory track:   ${MC_ENABLE_MEMORY}")
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
