/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "client/renderer/MeshTypes.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

namespace mc {
namespace client {
namespace renderer {

namespace trident {
namespace block {

// 前向声明（在 trident::block 命名空间）
class BreakProgressManager;

/**
 * @brief 破坏纹理渲染器
 *
 * 负责在方块上渲染破坏进度覆盖层。使用特殊的叠加混合模式
 * (DST_COLOR * SRC_COLOR) 将破坏纹理叠加到方块表面。
 *
 * 纹理来源：blocks atlas（AtlasManager 数据驱动）中的
 * block/destroy_stage_0..9 sprite。每阶段的 UV 在 updateMesh 时烘进顶点数据，
 * shader 直接用顶点 UV 采样，不再在 shader 内做 stage%5/stage/5 数学。
 *
 * 渲染流程：
 * 1. 从 BreakProgressManager 获取所有可见的破坏进度
 * 2. 为每个方块生成一个覆盖层立方体网格（UV 已按破坏阶段烘焙）
 * 3. 使用 blocks atlas 采样对应阶段的破坏纹理
 * 4. 通过叠加混合渲染到场景中
 */
class BreakProgressRenderer {
public:
    /**
     * @brief 顶点格式
     */
    struct Vertex {
        f32 x, y, z; // 位置
        f32 u, v;    // UV坐标（已烘焙为对应破坏阶段的绝对 UV）
    };

    /**
     * @brief 配置参数
     */
    struct Config {
        VkDevice device = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkDescriptorSetLayout cameraLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout fogLayout = VK_NULL_HANDLE;
        u32 maxFramesInFlight = 2;
    };

    /**
     * @brief 破坏阶段纹理区域查询回调
     *
     * 输入破坏阶段 (0-9)，返回 blocks atlas 中 block/destroy_stage_<stage> 的 UV 区域；
     * 未找到返回 nullptr（渲染器回退到全区域 UV）。
     */
    using StageRegionLookup = std::function<const TextureRegion*(u8 stage)>;

    /**
     * @brief 默认构造函数
     */
    BreakProgressRenderer() = default;

    /**
     * @brief 析构函数
     */
    ~BreakProgressRenderer();

    // 禁止拷贝
    BreakProgressRenderer(const BreakProgressRenderer&) = delete;
    BreakProgressRenderer& operator=(const BreakProgressRenderer&) = delete;

    /**
     * @brief 初始化渲染器
     *
     * 创建Vulkan资源：管线、缓冲区、描述符集等。
     * 纹理由 setBlockAtlas / setStageRegionLookup 在 AtlasManager 加载后注入。
     *
     * @param config 配置参数
     * @return 成功返回 true
     */
    [[nodiscard]] bool initialize(const Config& config, VkSampleCountFlagBits sampleCount);

    /**
     * @brief 清理资源
     */
    void cleanup();

    /**
     * @brief 注入 blocks atlas 的 GPU 句柄并写入纹理描述符集
     *
     * 由 TridentEngine 在 AtlasManager 加载/重载 blocks atlas 后调用。
     *
     * @param imageView blocks atlas 的图像视图
     * @param sampler   blocks atlas 的采样器
     */
    void setBlockAtlas(VkImageView imageView, VkSampler sampler);

    /**
     * @brief 注入破坏阶段纹理区域查询回调
     *
     * 由 TridentEngine 在 AtlasManager 加载 blocks atlas 后调用，
     * 绑定到 AtlasManager::findSpriteByTexturePath(block/destroy_stage_<stage>)。
     */
    void setStageRegionLookup(StageRegionLookup lookup) { m_stageRegionLookup = std::move(lookup); }

    /**
     * @brief 更新破坏进度网格
     *
     * 从 BreakProgressManager 获取所有可见的破坏进度，
     * 更新顶点缓冲区（每方块 UV 按其破坏阶段烘焙）。
     *
     * @param cameraPos 摄像机位置（用于距离裁剪）
     */
    void updateMesh(const Vector3& cameraPos);

    /**
     * @brief 渲染破坏覆盖层
     *
     * 使用叠加混合模式渲染所有破坏进度。
     * 应在区块渲染之后、GUI渲染之前调用。
     *
     * @param commandBuffer Vulkan命令缓冲区
     * @param cameraDescriptorSet 相机描述符集
     * @param fogDescriptorSet 雾效果描述符集
     */
    void render(VkCommandBuffer commandBuffer, VkDescriptorSet cameraDescriptorSet, VkDescriptorSet fogDescriptorSet);

    /**
     * @brief 检查是否有破坏进度需要渲染
     */
    [[nodiscard]] bool hasProgressToRender() const { return !m_progressEntries.empty(); }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    /**
     * @brief 破坏进度条目（用于渲染）
     */
    struct ProgressEntry {
        BlockPos position; // 方块位置
        u8 stage;          // 破坏阶段 (0-9)
        u32 vertexOffset;  // 该方块在顶点缓冲区中的起始偏移
        u32 indexOffset;   // 该方块在索引缓冲区中的起始偏移（以索引为单位）
    };

    /**
     * @brief 创建管线
     */
    [[nodiscard]] bool _createPipeline(VkSampleCountFlagBits sampleCount);

    /**
     * @brief 创建顶点/索引缓冲区
     */
    [[nodiscard]] bool _createBuffers();

    /**
     * @brief 创建描述符集
     */
    [[nodiscard]] bool _createDescriptorSets();

    /**
     * @brief 把 blocks atlas 的 imageView/sampler 写入纹理描述符集
     */
    void _writeBlockAtlasDescriptor(VkImageView imageView, VkSampler sampler);

    /**
     * @brief 生成立方体顶点数据
     *
     * 生成一个稍大于1的立方体（防止z-fighting）
     * 使用局部坐标（0-1范围），方块位置通过push constants传入。
     * 每面 0..1 局部 UV 映射到 stageRegion 的 u0/v0/u1/v1，把破坏阶段烘焙进顶点。
     *
     * @param cubeIndex 立方体索引（用于计算索引偏移）
     * @param stageRegion 该破坏阶段在 blocks atlas 中的 UV 区域
     * @param vertices 输出顶点数组
     * @param indices 输出索引数组
     */
    void _generateCubeMesh(
        size_t cubeIndex, const TextureRegion& stageRegion, std::vector<Vertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 确保缓冲区容量足够
     *
     * 如果当前缓冲区容量不足，重新创建更大的缓冲区。
     *
     * @param requiredVertices 所需顶点容量
     * @param requiredIndices 所需索引容量
     * @return 成功返回 true
     */
    [[nodiscard]] bool _ensureBufferCapacity(size_t requiredVertices, size_t requiredIndices);

    /**
     * @brief 重新创建缓冲区
     *
     * @param vertexCount 新的顶点容量
     * @param indexCount 新的索引容量
     * @return 成功返回 true
     */
    [[nodiscard]] bool _recreateBuffers(size_t vertexCount, size_t indexCount);

    /**
     * @brief 更新顶点缓冲区
     */
    void _updateVertexBuffer(const std::vector<Vertex>& vertices);

    /**
     * @brief 更新索引缓冲区
     */
    void _updateIndexBuffer(const std::vector<u32>& indices);

    // ========================================================================
    // 配置参数
    // ========================================================================

    Config m_config;

    // ========================================================================
    // Vulkan 资源
    // ========================================================================

    /// 管线布局
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

    /// 图形管线
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    /// 描述符集布局（纹理）
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

    /// 描述符池
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    /// 描述符集（纹理）
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    /// 顶点缓冲区
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;

    /// 顶点缓冲区内存
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;

    /// 索引缓冲区
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;

    /// 索引缓冲区内存
    VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;

    /// blocks atlas 句柄（由 setBlockAtlas 注入，纹理所有权归 AtlasManager）
    VkImageView m_blockImageView = VK_NULL_HANDLE;
    VkSampler m_blockSampler = VK_NULL_HANDLE;

    // ========================================================================
    // 渲染数据
    // ========================================================================

    /// 破坏阶段纹理区域查询回调（查 AtlasManager 的 blocks atlas）
    StageRegionLookup m_stageRegionLookup;

    /// 当前帧的破坏进度条目
    std::vector<ProgressEntry> m_progressEntries;

    /// 预分配的进度查询缓冲区（避免每帧分配）
    std::vector<std::pair<BlockPos, u8>> m_progressBuffer;

    /// 顶点数量
    size_t m_vertexCount = 0;

    /// 索引数量
    size_t m_indexCount = 0;

    /// 缓冲区最大容量
    size_t m_maxVertices = 0;
    size_t m_maxIndices = 0;

    /// 是否已初始化
    bool m_initialized = false;
};

} // namespace block
} // namespace trident
} // namespace renderer
} // namespace client
} // namespace mc
