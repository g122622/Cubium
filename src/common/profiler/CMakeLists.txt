# src/common/profiler/CMakeLists.txt
# 性能追踪库（Perfetto + Tracy 双轨）
#
# 两套 profiler 后端可独立开关、同时启用：
#   - MC_ENABLE_TRACING : Perfetto 后端（vendored 单文件 SDK，third_party/perfetto）
#   - MC_ENABLE_TRACY   : Tracy 后端（vendored submodule，third_party/tracy）
# 同时启用时，MC_TRACE_* 宏会同时向两套后端发事件（双轨录制）。
# 两者皆关时，mc_profiler 退化为 INTERFACE 空库，所有宏空展开、零开销。
#
# 对外暴露统一门面 mc::profiler::ProfilerManager（管理两套后端的生命周期与线程/进程命名）。

# ============================================================================
# Perfetto 后端（vendored 单文件 SDK，含 PR #6219 thread_ordering 支持）
# ============================================================================
if(MC_ENABLE_TRACING)
    # Perfetto SDK 单文件静态库（third_party/perfetto/perfetto.{h,cc}）
    # 独立 target 便于隔离 /bigobj 与禁警告，且不与业务代码增量编译耦合
    add_library(perfetto_sdk STATIC
        ${CMAKE_SOURCE_DIR}/third_party/perfetto/perfetto.cc
    )

    # BEFORE 强制插队首，防止 vcpkg 残留的旧版 perfetto.h 抢先被 include
    target_include_directories(perfetto_sdk BEFORE PUBLIC
        ${CMAKE_SOURCE_DIR}/third_party/perfetto
    )

    target_compile_features(perfetto_sdk PUBLIC cxx_std_20)

    # Windows 平台特殊配置
    if(WIN32)
        # Perfetto SDK 生成大量符号，按 Clang 前端类型选择大对象选项
        if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
            target_compile_options(perfetto_sdk PRIVATE "/bigobj" "/W0")
        else()
            target_compile_options(perfetto_sdk PRIVATE "-Wa,-mbig-obj" "-w")
        endif()

        # 禁用 Windows 遗留定义，避免与 Perfetto 冲突
        target_compile_definitions(perfetto_sdk PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
        )

        # 链接 WinSock2 与线程库（vcpkg port 之前隐式带，自编译需显式补）
        find_package(Threads REQUIRED)
        target_link_libraries(perfetto_sdk PRIVATE ws2_32 Threads::Threads)
    endif()
endif()

# ============================================================================
# Tracy 后端（vendored submodule，third_party/tracy）
# ============================================================================
# Tracy 的所有采集符号都在 #ifdef TRACY_ENABLE 守卫内，必须在编译其 client 库时
# 定义 TRACY_ENABLE（vcpkg 的 tracy port 不定义它，故走 vendored add_subdirectory）。
# add_subdirectory 后 Tracy::TracyClient 自带 PUBLIC TRACY_ENABLE，消费方无需再 define。
if(MC_ENABLE_TRACY)
    # TRACY_NO_SYSTEM_TRACING：Windows 下关闭 ETW context-switch 采集，否则需管理员权限运行。
    # 这些 CACHE 变量必须在 add_subdirectory 之前 FORCE 设置，覆盖上游 option 默认 OFF。
    set(TRACY_ENABLE ON CACHE BOOL "Enable Tracy profiling" FORCE)
    set(TRACY_NO_SYSTEM_TRACING ON CACHE BOOL "Disable ETW system tracing (avoid admin requirement)" FORCE)

    add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/tracy third_party_tracy EXCLUDE_FROM_ALL)

    # TracyClient.cpp 内联包含 10+ 个 .cpp（TracyProfiler/SysTrace/Socket 等），
    # 在 Clang+MSVC 下需 /bigobj 否则 section 数超限。上游 CMakeLists 未加，这里补。
    if(WIN32 AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(TracyClient PRIVATE "/bigobj")
    endif()
endif()

# ============================================================================
# 封装库 mc_profiler
# ============================================================================
# 至少一个后端启用时建 STATIC 库（含真实实现）；两者皆关时退化为 INTERFACE 空库
# （ProfilerManager 头文件提供内联存根，所有宏空展开）。
if(MC_ENABLE_TRACING OR MC_ENABLE_TRACY)
    set(_profiler_sources
        ProfilerManager.cpp
    )
    # Perfetto 后端源文件（含 PERFETTO_TRACK_EVENT_STATIC_STORAGE，必须且只能在一个 TU 定义）
    if(MC_ENABLE_TRACING)
        list(APPEND _profiler_sources
            PerfettoBackend.cpp
            TraceCategories.cpp
        )
    endif()

    add_library(mc_profiler STATIC ${_profiler_sources})

    target_link_libraries(mc_profiler
        PUBLIC
            spdlog::spdlog
    )

    # Perfetto 后端链接与编译定义
    if(MC_ENABLE_TRACING)
        # perfetto_sdk 必须 PUBLIC link：模板实例化符号须对依赖者可见
        target_link_libraries(mc_profiler PUBLIC perfetto_sdk)
        target_compile_definitions(mc_profiler PUBLIC MC_ENABLE_TRACING=1)
    endif()

    # Tracy 后端链接与编译定义（Tracy::TracyClient 自带 PUBLIC TRACY_ENABLE）
    if(MC_ENABLE_TRACY)
        target_link_libraries(mc_profiler PUBLIC Tracy::TracyClient)
        target_compile_definitions(mc_profiler PUBLIC MC_ENABLE_TRACY=1)
    endif()

    # 设置 C++20 标准
    target_compile_features(mc_profiler PUBLIC cxx_std_20)

    # 应用编译警告设置
    mc_set_compiler_warnings(mc_profiler)

    # 创建别名库，便于使用
    add_library(mc::profiler ALIAS mc_profiler)
else()
    # 两者皆关时创建空库
    add_library(mc_profiler INTERFACE)

    target_compile_definitions(mc_profiler INTERFACE
        MC_ENABLE_TRACING=0
        MC_ENABLE_TRACY=0
    )

    # 创建别名库
    add_library(mc::profiler ALIAS mc_profiler)
endif()
