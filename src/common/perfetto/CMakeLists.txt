# src/common/perfetto/CMakeLists.txt
# Perfetto 性能追踪库
#
# 使用 third_party/perfetto 下的官方单文件 SDK（含 PR #6219 thread_ordering 支持），
# 编译为独立的 perfetto_sdk 静态库，再由 mc_perfetto 封装。

# 根据追踪开关设置编译定义
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

    # 项目追踪库
    add_library(mc_perfetto STATIC
        PerfettoManager.cpp
        TraceCategories.cpp
    )

    # perfetto_sdk 必须 PUBLIC link：模板实例化符号须对依赖者可见
    target_link_libraries(mc_perfetto
        PUBLIC
            spdlog::spdlog
            perfetto_sdk
    )

    target_compile_definitions(mc_perfetto PUBLIC MC_ENABLE_TRACING=1)

    # 设置 C++20 标准
    target_compile_features(mc_perfetto PUBLIC cxx_std_20)

    # 应用编译警告设置
    mc_set_compiler_warnings(mc_perfetto)

    # 创建别名库，便于使用
    add_library(mc::perfetto ALIAS mc_perfetto)
else()
    # 追踪禁用时创建空库
    add_library(mc_perfetto INTERFACE)

    target_compile_definitions(mc_perfetto INTERFACE MC_ENABLE_TRACING=0)

    # 创建别名库
    add_library(mc::perfetto ALIAS mc_perfetto)
endif()
