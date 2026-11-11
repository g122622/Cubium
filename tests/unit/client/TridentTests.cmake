# Trident 需要独立的 GoogleTest main，与普通客户端测试保持进程隔离。
# Trident 渲染引擎测试可执行文件
# 注意：这里只测试核心组件，不测试 TridentEngine 本体
add_executable(mc_trident_tests
    client/renderer/test_trident_engine.cpp
    client/renderer/PolarBearAnimationTest.cpp
    client/renderer/EntityRendererCoverageTest.cpp
    client/renderer/AnimatedMeshCacheTest.cpp
)

# 添加 Trident 渲染引擎核心组件源文件
target_sources(mc_trident_tests
    PRIVATE
        # Trident 核心（不含 TridentEngine）
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/TridentContext.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/TridentSwapchain.cpp
        # Trident 缓冲区
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/buffer/TridentBuffer.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/buffer/TridentStagingBufferPool.cpp
        # Trident 纹理
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/texture/TridentTexture.cpp
        # Trident 绠＄嚎
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/pipeline/TridentPipeline.cpp
        # Trident 渲染管理器
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/render/FrameManager.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/render/UniformManager.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/render/DescriptorManager.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/core/render/RenderPassManager.cpp
        # 资源加载
        ${CMAKE_SOURCE_DIR}/src/client/resource/TextureAtlasBuilder.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ItemModelCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ItemModelLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/ResourceManager.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockModelLoader.cpp
        ${CMAKE_SOURCE_DIR}/src/client/resource/BlockStateLoader.cpp
        # 缃戞牸鐩稿叧
        ${CMAKE_SOURCE_DIR}/src/client/renderer/MeshTypes.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/chunk/ChunkMesher.cpp
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/chunk/AmbientOcclusionCalculator.cpp
        # 生物群系颜色
        ${CMAKE_SOURCE_DIR}/src/client/world/color/BiomeColors.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/color/blend/BiomeColorCache.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/color/blend/BiomeColorBlender.cpp
        ${CMAKE_SOURCE_DIR}/src/client/world/color/blend/ChunkBiomeAccessor.cpp
        # 常用资源
        # 方块系统
        # 设置
        ${CMAKE_SOURCE_DIR}/src/client/settings/ClientSettings.cpp
        # 按键绑定（自 common/input 迁入 client/input）
        ${CMAKE_SOURCE_DIR}/src/client/input/KeyBinding.cpp
        # AnimationContext
        ${CMAKE_SOURCE_DIR}/src/client/renderer/trident/entity/core/AnimationContext.cpp
)

target_link_libraries(mc_trident_tests
    PRIVATE
        mc_gen
        mc_common
        mc_bedrock_addon
        mc_offset_allocator
        GTest::gtest_main
        glfw
        Vulkan::Vulkan
        nlohmann_json::nlohmann_json
        glm::glm
)

target_include_directories(mc_trident_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)

mc_disable_all_warnings_for_tests(mc_trident_tests)
mc_copy_runtime_dlls(mc_trident_tests)

mc_register_gtests(mc_trident_tests)

set_target_properties(mc_trident_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
