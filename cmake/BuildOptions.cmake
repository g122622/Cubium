# 编译选项
option(MC_BUILD_CLIENT "Build client application" ON)
option(MC_BUILD_SERVER "Build server application" ON)
option(MC_BUILD_TESTS "Build unit tests" ON)
option(MC_BUILD_CLIENT_TESTS "Build the separate client unit test targets" OFF)
option(MC_BUILD_BENCHMARKS "Build benchmark application" OFF)
# 协议模糊测试目标（libFuzzer harness，源码在 tests/fuzz）。
# 默认 OFF：fuzz 目标需要 clang 的 -fsanitize=fuzzer 运行时，且只在专门的 fuzz preset
# 下构建（见 docs/test/FUZZING.md）。开启后目标均标 EXCLUDE_FROM_ALL，不参与常规构建。
option(MC_BUILD_FUZZERS "Build libFuzzer protocol fuzz targets (tests/fuzz)" OFF)
# 差分测试探针（tests/diff）：同一份输入喂给 Cubium 与参照实现，要求输出逐字节相同。
# 默认 OFF：依赖原版数据（minecraft-data 的 blocks.json 等）与 Java oracle，属于专门的
# 一致性验证流程。开启后目标仍为 EXCLUDE_FROM_ALL，须显式 --target diff_probes 构建。
option(MC_BUILD_DIFF_PROBES "Build differential probes (tests/diff)" OFF)
option(MC_ENABLE_SANITIZERS "Enable sanitizers for the CI sanitizer builds" OFF)
# 生产 target 禁用 C++ 异常（-fno-exceptions，Windows 另需 _HAS_EXCEPTIONS=0）。
#
# 打开后：生产 target 的断言一律走 defaultAssertHandler（abort + 诊断输出），
# tests target 仍保留异常（gtest 的 EXPECT_THROW 与 AssertException 支持依赖它）。
# 详见 docs/iterations/exception禁用指南.md。
option(MC_ENABLE_NO_EXCEPTIONS "Compile production targets with -fno-exceptions" OFF)
# 灰度开关：为空表示「全部生产 target」；填分号分隔的 target 名则只对子集生效。
# 用途是分 target 逐步去异常化（例如先只给 mc_common 打开，验证通过后再扩到 server）。
set(MC_NO_EXCEPTIONS_TARGETS "" CACHE STRING
    "MC_ENABLE_NO_EXCEPTIONS 生效的 target 子集（分号分隔）；留空表示全部生产 target")
option(MC_ENABLE_VULKAN_VALIDATION "Enable Vulkan validation layers" OFF)
option(MC_ENABLE_TRACING "Enable Perfetto tracing" ON)
# Tracy 默认关闭：它带来可观的常驻内存开销，且不经系统 malloc（无法用常规手段观测）。
# 代价是 Windows 上 Tracy 的 SymbolWorker 会调 SymInitialize/SymLoadModuleEx 把全部模块的
# 调试符号载入堆（实测约 96 MB 且不释放），其自带的 rpmalloc 还会按 4 MiB 为单位 VirtualAlloc
# 做 span map。本项目以 Perfetto 为主要追踪后端，故 Tracy 需显式开启。
option(MC_ENABLE_TRACY "Enable Tracy profiling" OFF)
# Tracy 默认在无 GUI 连接时也把事件写入进程内队列，而该队列只在 GUI 连接时才被排空，
# 长驻服务端下会无限增长（实测约 200KB/s）。ON_DEMAND 让无连接时不采集，消除该增长。
# 仅 MC_ENABLE_TRACY=ON 时被消费。
option(MC_TRACY_ON_DEMAND "Enable Tracy on-demand profiling (collect only while a GUI is connected)" ON)
option(MC_ENABLE_MEMORY "Enable memory allocation tracking via Tracy" OFF)
option(MC_ENABLE_CCACHE "Enable ccache for faster rebuilds" ON)
# 针对本机 CPU 生成优化指令（-march=native -mtune=native）。
#
# 默认 ON 适合「在本机编译并在本机运行」的日常开发。**跨机器场景必须关闭**：
# -march=native 会按编译机的 CPU 特性（AVX-512 等）生成指令，产物在缺少该特性的
# 机器上执行会触发 SIGILL（Illegal instruction）。
#
# CI 是典型跨机器场景：build job 与各测试 job 跑在**不同的** runner 实例上，
# 因此 CI 的 configure 必须显式传 -DMC_ENABLE_NATIVE_ARCH=OFF。
option(MC_ENABLE_NATIVE_ARCH "Optimize for the build machine's CPU (-march=native)" ON)


if(MC_BUILD_CLIENT_TESTS AND NOT MC_BUILD_TESTS)
    message(FATAL_ERROR "MC_BUILD_CLIENT_TESTS requires MC_BUILD_TESTS=ON")
endif()

# vcpkg 在 project() 内安装 manifest，功能选择必须在它之前完成。
set(VCPKG_MANIFEST_NO_DEFAULT_FEATURES ON)
set(VCPKG_MANIFEST_FEATURES "")
if(MC_BUILD_SERVER)
    list(APPEND VCPKG_MANIFEST_FEATURES server)
endif()
if(MC_BUILD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES tests)
endif()
if(MC_BUILD_BENCHMARKS)
    list(APPEND VCPKG_MANIFEST_FEATURES benchmarks)
endif()
if(MC_BUILD_CLIENT OR MC_BUILD_CLIENT_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES client)
endif()
