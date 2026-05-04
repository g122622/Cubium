# src/common/perfetto/CMakeLists.txt
# Perfetto 性能追踪库
#
# 按照 Perfetto SDK 官方文档的建议，perfetto.cc 编译为独立的静态库

# 根据追踪开关设置编译定义
if(MC_ENABLE_TRACING)
    # Perfetto SDK 静态库 (按照官方文档建议)
    add_library(perfetto_sdk STATIC
        ${CMAKE_SOURCE_DIR}/third_party/perfetto/perfetto.cc
    )

    target_include_directories(perfetto_sdk
        PUBLIC
            ${CMAKE_SOURCE_DIR}/third_party/perfetto
    )

    # 定义 PERFETTO_IMPLEMENTATION 以确保符号正确导出
    target_compile_definitions(perfetto_sdk
        PRIVATE
            PERFETTO_IMPLEMENTATION
    )

    # Windows 平台特殊配置
    if(WIN32)
        # Perfetto SDK 生成大量符号，需要 /bigobj 选项
        target_compile_options(perfetto_sdk PRIVATE "/bigobj")

        # 禁用 Windows 遗留定义，避免与 Perfetto 冲突
        target_compile_definitions(perfetto_sdk PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
        )

        # 链接 WinSock2
        target_link_libraries(perfetto_sdk PUBLIC ws2_32)
    endif()

    # 启用标准合规模式 (MSVC)
    if(MSVC)
        target_compile_options(perfetto_sdk PRIVATE "/permissive-")
    endif()

    # 设置 C++20 标准
    target_compile_features(perfetto_sdk PUBLIC cxx_std_20)

    # 创建别名库
    add_library(mc::perfetto_sdk ALIAS perfetto_sdk)
endif()

# 项目追踪库
add_library(mc_perfetto STATIC
    PerfettoManager.cpp
    TraceCategories.cpp
)

target_include_directories(mc_perfetto
    PUBLIC
        ${CMAKE_SOURCE_DIR}/third_party/perfetto
)

target_link_libraries(mc_perfetto
    PUBLIC
        spdlog::spdlog
)

# Windows 平台特殊配置
if(WIN32)
    # Perfetto SDK 生成大量符号，需要 /bigobj 选项
    target_compile_options(mc_perfetto PRIVATE "/bigobj")

    # 禁用 Windows 遗留定义，避免与 Perfetto 冲突
    target_compile_definitions(mc_perfetto PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )

    # 链接 WinSock2
    target_link_libraries(mc_perfetto PRIVATE ws2_32)
endif()

# 根据追踪开关设置编译定义
if(MC_ENABLE_TRACING)
    target_compile_definitions(mc_perfetto PUBLIC MC_ENABLE_TRACING=1)
    # perfetto_sdk 链接必须是 PUBLIC 以确保其符号对所有依赖者可用
    target_link_libraries(mc_perfetto PUBLIC perfetto_sdk)
else()
    target_compile_definitions(mc_perfetto PUBLIC MC_ENABLE_TRACING=0)
endif()

# 设置 C++20 标准
target_compile_features(mc_perfetto PUBLIC cxx_std_20)

# 应用编译警告设置
mc_set_compiler_warnings(mc_perfetto)

# 创建别名库，便于使用
add_library(mc::perfetto ALIAS mc_perfetto)
