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

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildOptions.cmake")

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

# ============================================================================
# 构建配置与调试信息等级
#
# 本项目只支持 RelWithDebInfo 与 Release 两种配置。Debug（-O0）在区块生成、网络
# 包编解码等热路径上慢一个数量级，实测没有可用性，已从 preset、CI 与文档中彻底移除。
#
# RelWithDebInfo 的调试信息只保留「文件 / 行号 / 函数名」：
# 默认的 -g 会为每个模板实例化、每个内联副本生成完整的类型与变量 DIE。本工程
# 单个 .cpp 的 .o 曾达 188 MB，其中 __debug_str（类型名字符串表）占 142 MB，而代码段
# 仅 0.9 MB；整个 build/ 曾膨胀到 74 GB。改用 -gline-tables-only 后同一 .o 降到 41 MB
# （-78%），单 TU 编译时间也下降约 25%，同时仍能给出崩溃栈的文件与行号。
# -gno-pubnames 去掉 __debug_names 加速表（调试器按名字查类型用，行号级调试不需要）。
# 需要观察变量值时按项目规范用日志输出，不依赖调试器。
# ============================================================================
set(CMAKE_CONFIGURATION_TYPES "RelWithDebInfo;Release" CACHE STRING "Supported build configurations" FORCE)

set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -DNDEBUG -gline-tables-only -gno-pubnames")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -DNDEBUG -gline-tables-only -gno-pubnames")

# PCH 与 ccache 的兼容性（Clang 专用）。
#
# Clang 会把 PCH 的创建时间戳写进 .pch，使用方编译时校验该时间戳。CI 每次运行都在新容器
# 里重建 PCH（时间戳必然不同），若不做处理，所有使用 PCH 的 TU 都会被 ccache 判定为
# 「不可缓存」（ccache 统计中的 Could not use precompiled header），整条缓存链路形同虚设。
# 实测：CI 日志里该项占比 99.75%。
#
# 官方要求的配置见 ccache 手册「Precompiled headers」：
#   1. -Xclang -fno-pch-timestamp（本处）——PCH 不再内嵌时间戳，跨机器/跨次构建可比；
#   2. CCACHE_SLOPPINESS 含 pch_defines,time_macros（CompilerCache 模块随 launcher 设置）。
# 二者缺一不可：只设 sloppiness 时命中率仅 ~17%，加上本选项后可达 ~83%（WSL 实测）。
#
# 【必须放在顶层，不能放进 src/common/CMakeLists.txt】`add_compile_options` 只对**其后
# 创建**的目标生效。而 mc_common 在 src/common/CMakeLists.txt 开头
# 就创建了，在该文件里添加只会命中文件末尾才创建的 mc_bedrock_addon，等于没加。
# 放在此处则在 add_subdirectory 之前，对全部子目录生效。
#
# 该选项是 Clang 的 no-op 安全开关：不使用 PCH 的 TU（如 mc_gen / mc_benchmark）
# 带上它后产物与不带时逐字节相同，故统一添加，无需按目标区分。
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Xclang -fno-pch-timestamp)
endif()

if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# CMake模块
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# 编译配置
include(CompilerWarnings)
include(BuildScheduling)
include(CompilerCache)
include(Sanitizers)

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
# 多配置生成器（Ninja Multi-Config）下 CMAKE_BUILD_TYPE 为空，实际配置由 --config 决定；
# 此时按默认 preset 使用的配置记为 RelWithDebInfo，避免版本信息里误写成 Release。
if(CMAKE_BUILD_TYPE STREQUAL "")
    set(MC_BUILD_TYPE "RelWithDebInfo")
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
# 注意：不能用 CMAKE_SIZEOF_VOID_P 判定架构——指针宽度只能区分 64/32 位，
# 会把 arm64（macOS Apple Silicon、Linux aarch64）误报成 x64。
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|ARM64|aarch64|AARCH64)$")
    set(MC_BUILD_ARCH "arm64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(MC_BUILD_ARCH "x64")
else()
    set(MC_BUILD_ARCH "x86")
endif()

# 查找依赖包
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(simdjson CONFIG REQUIRED)
if(MC_BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)
endif()
find_package(asio CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(LibArchive REQUIRED)
find_package(ZLIB REQUIRED)
find_package(RocksDB CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)

# vcpkg 的 MSVC 构建把链接器选项写入了链接库列表；clang++ 需要通过 LINKER 转发。
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
        AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    get_property(_abslImportedTargets DIRECTORY PROPERTY IMPORTED_TARGETS)
    foreach(_abslTarget IN LISTS _abslImportedTargets)
        if(_abslTarget MATCHES "^absl::")
            get_target_property(_abslLinkLibraries ${_abslTarget} INTERFACE_LINK_LIBRARIES)
            if(_abslLinkLibraries)
                string(REPLACE "-ignore:4221" "" _abslLinkLibraries "${_abslLinkLibraries}")
                set_property(TARGET ${_abslTarget} PROPERTY INTERFACE_LINK_LIBRARIES "${_abslLinkLibraries}")
            endif()
        endif()
    endforeach()
    target_link_options(absl::abseil_dll INTERFACE "LINKER:/ignore:4221")
endif()
find_package(leveldb CONFIG REQUIRED)
find_package(gflags CONFIG REQUIRED)
find_package(asmjit CONFIG REQUIRED)

# 图形与音频依赖仅属于客户端及其独立测试。
if(MC_BUILD_CLIENT OR MC_BUILD_CLIENT_TESTS)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(Vulkan REQUIRED)
    find_package(OpenAL CONFIG REQUIRED)
endif()

if(MC_BUILD_BENCHMARKS)
    find_package(benchmark CONFIG REQUIRED)
endif()

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

# 所有平台统一使用 LLVM lld：Linux 用 ld.lld、Windows 用 lld-link、macOS 用 ld64.lld。
#
# 本项目的产物体积很大（单测二进制在 RelWithDebInfo + ASan 下约 2.3 GB），GNU ld 链接这种
# 二进制要两分钟以上，MSVC link.exe 也明显慢于 lld。
#
# CMake 4.2 在 Windows + Clang 下默认已经选择 lld-link（此时 CMAKE_LINKER 即 lld-link.exe，
# 链接命令自带 -fuse-ld=lld-link），不要重复追加。其余平台显式传入 lld 可执行文件的绝对路径，
# 而不是裸名 -fuse-ld=lld：Apple Clang 不识别裸名（只接受已存在的路径），绝对路径也能避免
# PATH 中混入旧版本的 lld。找不到 lld 时直接失败，避免静默退回平台默认链接器。
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    get_filename_component(_mc_linker_name "${CMAKE_LINKER}" NAME)
    if(_mc_linker_name MATCHES "lld")
        message(STATUS "Linker: lld (${CMAKE_LINKER})")
    else()
        if(WIN32)
            # Visual Studio 自带的 LLVM 把 lld-link.exe 与 clang 放在同一目录。
            get_filename_component(_mc_clang_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
            find_program(MC_LLD_EXECUTABLE NAMES lld-link HINTS "${_mc_clang_bin_dir}")
        elseif(APPLE)
            # Homebrew 自 LLVM 19 起把 lld 拆成独立 formula（brew install lld）。
            find_program(MC_LLD_EXECUTABLE NAMES ld64.lld HINTS /opt/homebrew/opt/lld/bin /usr/local/opt/lld/bin)
        else()
            find_program(MC_LLD_EXECUTABLE NAMES ld.lld)
        endif()

        if(NOT MC_LLD_EXECUTABLE)
            message(FATAL_ERROR "LLVM lld not found for ${CMAKE_SYSTEM_NAME}; install it or pass -DMC_LLD_EXECUTABLE=<path>")
        endif()

        add_link_options("-fuse-ld=${MC_LLD_EXECUTABLE}")
        message(STATUS "Linker: lld (${MC_LLD_EXECUTABLE})")
    endif()
endif()

if(MC_ENABLE_SANITIZERS)
    # Sanitizer 构建：降低优化等级并关闭可能干扰诊断的选项
    # 具体插桩由 Sanitizers 模块统一添加，不依赖通用 flags 缓存。
    add_compile_options(-O1 -fno-omit-frame-pointer -fno-common)
    add_compile_options(-fno-fast-math) # sanitizer 需要精确浮点语义
    # Sanitizer 插桩可能阻止显式请求的向量化；非 sanitizer 构建仍保留优化失败诊断。
    add_compile_options(-Wno-pass-failed)
    message(STATUS "Sanitizer build: ASan/UBSan enabled with -O1 and frame pointers")
else()
    add_compile_options(-O2) # 不开o3，o3对缓存不友好、增加编译时间且可能存在bug
    # 向量化请求失败（`#pragma clang loop vectorize_width`）只报告为警告。本工程的浮点
    # 语义依赖 `-ffast-math`，同一份代码在不同优化路径（内联深度、常量传播）下能否向量化
    # 并不稳定；把它升级成 -Werror 会让构建随编译器启发式偶发失败（PerlinNoise /
    # BlendedNoise 的噪声采样循环就出现过）。性能回归由基准测试把关，不靠编译失败把关。
    add_compile_options(-Wno-pass-failed)
    # 仅当明确要求时针对本机 CPU 生成指令（见 MC_ENABLE_NATIVE_ARCH 的说明）。
    # 跨机器分发产物（CI 的 build job → 测试 job）时必须关闭，否则可能在无对应
    # CPU 特性的机器上触发 SIGILL。
    if(MC_ENABLE_NATIVE_ARCH)
        add_compile_options(-march=native -mtune=native) # 架构专用优化：针对本地 CPU 生成优化指令
    endif()
    add_compile_options(-ffast-math -fno-finite-math-only) # 启用快速浮点数，但保留 NaN/Inf 语义以允许 isfinite 检查
    add_compile_options(-fno-trapping-math) # 禁用浮点异常
    # AVX2 是 x86_64 上近乎通用的基线（Haswell 2013+ / Excavator 2015+），
    # 即便关闭 native 优化也保留，作为项目对 x86_64 的最低要求。
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
        add_compile_options(-mavx2) # 启用 AVX2 指令集（仅 x86_64，arm64 不支持）
    endif()
    add_compile_options(-fno-stack-protector -fno-stack-check) # 关闭栈保护，减少性能开销（注意安全风险）
    # 链接时优化。只在 Release 配置启用。
    # 多配置生成器下不能用 CMAKE_BUILD_TYPE 判断：它是 configure 时的单一值，而
    # add_compile_options 会作用到所有配置，导致 RelWithDebInfo 也吃进 -flto。
    add_compile_options($<$<CONFIG:Release>:-flto=thin>) # 启用链接时优化（LTO），使用 thin 模式以减少编译时间
    add_link_options($<$<CONFIG:Release>:-flto=thin>)
    # 链接器本身在文件上方按平台选择（全平台使用 lld）。
    # 移除未引用的函数和数据：GNU 风格为 --gc-sections，MSVC 风格为 /OPT:REF。
    if(WIN32)
        add_link_options($<$<CONFIG:Release>:/OPT:REF>)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_link_options($<$<CONFIG:Release>:-Wl,--gc-sections>)
    endif()
endif()

# 崩溃调用栈的符号化（Linux）。
#
# CrashHandler 用 backtrace_symbols() 取栈帧符号，而该函数只查**动态符号表**（.dynsym）。
# 可执行文件的函数默认只进 .symtab，于是崩溃日志里全是
#     minecraft-server(+0xdcb55c) [0x55ba8c22755c]
# 这种「只有偏移量、没有函数名」的帧，必须事后拿二进制用 addr2line/llvm-symbolizer
# 逐个换算才能定位，排查成本极高（尤其 CI 上只能靠日志）。
#
# -rdynamic（等价于 -Wl,--export-dynamic）把可执行文件的全局符号也导出到 .dynsym，
# backtrace_symbols 即可直接给出函数名。C++ 名字由 CrashHandler 用 __cxa_demangle 还原。
# 实测对比：不加为 `./bin(+0x122f)`，加了为 `./bin(main+0x1f)`。
#
# 代价是二进制略增大（导出符号表），服务端约数 MB，相对其体积可忽略。
# macOS 的 ld64 不支持该选项（且默认已导出全部符号），故仅在 Linux 上加。
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_link_options(-rdynamic)
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
    # 单元测试源码统一收在 tests/unit 下（tests/integrated 为行为包集成测试数据，无 CMake 构建）
    add_subdirectory(tests/unit)
endif()

if(MC_BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()

if(MC_BUILD_FUZZERS)
    # 协议模糊测试目标（libFuzzer）。目标均为 EXCLUDE_FROM_ALL：不参与 `cmake --build`
    # 的全量构建，须显式 --target 指定。构建与运行方式见 docs/test/FUZZING.md。
    add_subdirectory(tests/fuzz)
endif()

if(MC_BUILD_DIFF_PROBES)
    # 差分测试探针。同样 EXCLUDE_FROM_ALL，须显式 --target diff_probes。
    # 构建与运行方式见 tests/diff/README.md。
    add_subdirectory(tests/diff)
endif()

# ============================================================================
# 去异常化开关（MC_ENABLE_NO_EXCEPTIONS）
# ============================================================================
#
# 只作用于**生产 target**：test target 必须保留异常（gtest 的 EXPECT_THROW、
# tests/unit/common/AssertExceptionSupport.cpp 提供的断言异常机制都依赖它）。
# 不能放进顶层 add_compile_options：那会连测试一起禁用异常，导致测试目标编译失败。
if(MC_ENABLE_NO_EXCEPTIONS)
    # 注意：真正编译 src/ 代码的是这些库 target，而不是可执行文件。
    # 可执行文件只含各自的 main/入口。只给可执行文件加 -fno-exceptions 是**无效**的。
    set(_mc_no_exception_targets mc_common)
    if(TARGET mc_gen)
        list(APPEND _mc_no_exception_targets mc_gen)
    endif()
    if(TARGET mc_server_shared)
        list(APPEND _mc_no_exception_targets mc_server_shared)
    endif()
    if(TARGET mc_server_storage)
        list(APPEND _mc_no_exception_targets mc_server_storage)
    endif()
    if(TARGET mc_bedrock_addon)
        list(APPEND _mc_no_exception_targets mc_bedrock_addon)
    endif()
    if(MC_BUILD_SERVER)
        list(APPEND _mc_no_exception_targets minecraft-server)
    endif()
    # 注意：mc_benchmark **故意不纳入**去异常范围。它含 benchmark/MemoryProfiler.cpp 里的
    # 全局 operator new 替换（统计分配量、失败时 throw std::bad_alloc，并实现 nothrow 重载），
    # 这套脚手架本质上依赖异常语义；它是压测工具而非交付产物，保留异常不影响可发布二进制。
    if(MC_BUILD_BENCHMARKS)
        message(STATUS "  No exceptions: mc_benchmark excluded (custom operator new needs exceptions)")
    endif()

    # 灰度子集：显式指定时只对列出的 target 生效（仍会与上面求交集，避免写错 target 名静默生效）。
    if(MC_NO_EXCEPTIONS_TARGETS)
        set(_mc_requested_targets ${MC_NO_EXCEPTIONS_TARGETS})
        set(_mc_filtered_targets "")
        foreach(_mc_target IN LISTS _mc_no_exception_targets)
            if(_mc_target IN_LIST _mc_requested_targets)
                list(APPEND _mc_filtered_targets ${_mc_target})
            endif()
        endforeach()
        foreach(_mc_requested IN LISTS _mc_requested_targets)
            if(NOT _mc_requested IN_LIST _mc_filtered_targets)
                message(WARNING "MC_NO_EXCEPTIONS_TARGETS 里的 '${_mc_requested}' 不是生产 target，已忽略")
            endif()
        endforeach()
        set(_mc_no_exception_targets ${_mc_filtered_targets})
    endif()
    message(STATUS "  No exceptions targets: ${_mc_no_exception_targets}")

    foreach(_mc_target IN LISTS _mc_no_exception_targets)
        if(TARGET ${_mc_target})
            target_compile_options(${_mc_target} PRIVATE -fno-exceptions)
            if(MSVC)
                # MSVC 的 STL 用 _HAS_EXCEPTIONS 控制内部 throw；-fno-exceptions 不会自动置位，
                # 不定义时 STL 头文件自身会编译失败（见 docs/iterations/exception禁用指南.md）。
                target_compile_definitions(${_mc_target} PRIVATE _HAS_EXCEPTIONS=0)
            endif()
            # entt 是 header-only，其 config.h 依据 __cpp_exceptions 发出
            #   #pragma detect_mismatch("entt.noexcept", ENTT_TRY 的字符串形式)
            # 于是开了异常的 TU 得到 "try"、关了的得到 "if(true)"，混在同一个链接里
            # 会被 lld 的 /failifmismatch 拒绝。统一改为定义 ENTT_NOEXCEPTION，
            # 让所有包含 entt 的 TU 发出同一个值（见下方 mc_tests 的同样处理）。
            target_compile_definitions(${_mc_target} PRIVATE ENTT_NOEXCEPTION)
        endif()
    endforeach()

    # 测试和基准目标保留真实异常，但同样会包含 entt 头文件并链接上面这些
    # -fno-exceptions 的静态库。只要有一个 TU 的 ENTT_TRY 取到 "try"，
    # 链接就会报 entt.noexcept 不一致。所以凡是参与测试链接的目标都补同一个定义。
    # ENTT_NOEXCEPTION 只影响 entt 内部的 throw 策略，不影响调用方能否使用 try/throw。
    foreach(_mc_test_target mc_tests mc_test_support mc_client_tests mc_trident_tests mc_benchmark diff_runner)
        if(TARGET ${_mc_test_target})
            target_compile_definitions(${_mc_test_target} PRIVATE ENTT_NOEXCEPTION)
        endif()
    endforeach()
    message(STATUS "  No exceptions:  ON (production targets only)")
else()
    message(STATUS "  No exceptions:  OFF")
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
message(STATUS "  Build Fuzzers:  ${MC_BUILD_FUZZERS}")
message(STATUS "  Sanitizers:     ${MC_ENABLE_SANITIZERS}")
message(STATUS "  Vulkan Val:     ${MC_ENABLE_VULKAN_VALIDATION}")
message(STATUS "  Tracing:        ${MC_ENABLE_TRACING}")
message(STATUS "  Tracy:          ${MC_ENABLE_TRACY}")
message(STATUS "  Tracy OnDemand: ${MC_TRACY_ON_DEMAND}")
message(STATUS "  Memory track:   ${MC_ENABLE_MEMORY}")
if(MC_ENABLE_CCACHE AND CCACHE_PROGRAM)
    message(STATUS "  ccache:         Enabled (${CCACHE_PROGRAM})")
elseif(MC_ENABLE_CCACHE)
    message(STATUS "  ccache:         Not found")
else()
    message(STATUS "  ccache:         Disabled")
endif()
message(STATUS "======================================")
message(STATUS "")
