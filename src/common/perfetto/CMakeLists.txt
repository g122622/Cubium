# src/common/perfetto/CMakeLists.txt
# Perfetto 性能追踪库
#
# 使用 vcpkg 安装的 perfetto 库

# 根据追踪开关设置编译定义
if(MC_ENABLE_TRACING)
    # 项目追踪库
    add_library(mc_perfetto STATIC
        PerfettoManager.cpp
        TraceCategories.cpp
    )

    target_link_libraries(mc_perfetto
        PUBLIC
            spdlog::spdlog
            unofficial::perfetto::perfetto
    )

    # Windows 平台特殊配置
    if(WIN32)
        # Perfetto SDK 生成大量符号，按 Clang 前端类型选择大对象选项
        if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
            target_compile_options(mc_perfetto PRIVATE "/bigobj")
        else()
            target_compile_options(mc_perfetto PRIVATE "-Wa,-mbig-obj")
        endif()

        # 禁用 Windows 遗留定义，避免与 Perfetto 冲突
        target_compile_definitions(mc_perfetto PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
        )

        # 链接 WinSock2
        target_link_libraries(mc_perfetto PRIVATE ws2_32)
    endif()

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
