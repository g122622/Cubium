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

#include "client/renderer/trident/cloud/CloudMode.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <vector>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc {
class ResourceManager;
}

namespace mc::client::renderer::trident::cloud {

/**
 * @brief 云 Uniform 缓冲区数据结构
 *
 * 传递给云着色器的 Uniform 数据。
 */
struct CloudUBO {
    alignas(16) glm::vec4 cloudColor; ///< 云颜色 (RGBA)
    alignas(4) f32 cloudHeight;       ///< 云高度 (192.0f 主世界)
    alignas(4) f32 time;              ///< 时间（用于动画）
    alignas(4) f32 textureScale;      ///< 纹理缩放因子
    alignas(4) f32 cameraY;           ///< 相机 Y 坐标（用于高度雾）
    alignas(4) f32 textureOffsetX;    ///< 纹理 X 偏移（基于相机位置）
    alignas(4) f32 textureOffsetZ;    ///< 纹理 Z 偏移（基于相机位置）
};

/**
 * @brief 云渲染器
 *
 * 负责渲染天空中的云层。
 *
 * 渲染模式：
 * - Fast: 只渲染云底面，性能优化
 * - Fancy: 渲染完整 3D 云体，包含顶部、底部和四个侧面
 *
 * 云特性：
 * - 云高度：主世界 192 格，下界和末地无云
 * - 云随时间缓慢移动
 * - 云颜色随时间和天气变化
 */
class CloudRenderer {
public:
    CloudRenderer();
    ~CloudRenderer();

    // 禁止拷贝
    CloudRenderer(const CloudRenderer&) = delete;
    CloudRenderer& operator=(const CloudRenderer&) = delete;

    // 允许移动
    CloudRenderer(CloudRenderer&& other) noexcept;
    CloudRenderer& operator=(CloudRenderer&& other) noexcept;

    // ========================================================================
    // 初始化与销毁
    // ========================================================================

    /**
     * @brief 初始化云渲染器
     *
     * @param device Vulkan 逻辑设备
     * @param physicalDevice Vulkan 物理设备
     * @param commandPool 命令池
     * @param graphicsQueue 图形队列
     * @param renderPass 渲染通道
     * @param extent 交换链图像尺寸
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        VkRenderPass renderPass,
        VkExtent2D extent,
        VkSampleCountFlagBits sampleCount,
        const ResourceManager* resourceManager);

    /**
     * @brief 热更新云纹理资源
     *
     * 从资源包重新读取 clouds.png 并替换 GPU 纹理。
     *
     * @param resourceManager 资源管理器（允许为空；为空时使用程序化纹理回退）
     * @return 成功或错误
     *
     * @warning 需要在渲染器已初始化后调用。
     */
    [[nodiscard]] Result<void> reloadTexture(const ResourceManager* resourceManager = nullptr);

    /**
     * @brief 销毁所有资源
     */
    void destroy();

    /**
     * @brief 窗口大小变化时重新创建资源
     * @param extent 新的交换链图像尺寸
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> onResize(VkExtent2D extent);

    // ========================================================================
    // 更新与渲染
    // ========================================================================

    /**
     * @brief 更新云状态
     *
     * 计算云的位置偏移和颜色。
     *
     * @param dayTime 当前一天内时间 (0-23999)
     * @param gameTime 游戏总 tick 数
     * @param partialTick 部分 tick（用于插值）
     * @param cloudHeight 云高度（维度相关，NaN 表示无云）
     * @param cloudColor 云颜色（来自 SkyRenderer）
     */
    void update(i64 dayTime, i64 gameTime, f64 partialTick, f64 cloudHeight, const glm::vec4& cloudColor);

    /**
     * @brief 渲染云
     *
     * @param cmd 命令缓冲区
     * @param projection 相机投影矩阵
     * @param view 相机视图矩阵
     * @param cameraPos 相机位置
     * @param mode 云渲染模式
     * @param frameIndex 当前帧索引
     */
    void render(VkCommandBuffer cmd,
        const glm::mat4& projection,
        const glm::mat4& view,
        const glm::vec3& cameraPos,
        CloudMode mode,
        u32 frameIndex);

    // ========================================================================
    // 状态查询
    // ========================================================================

    /**
     * @brief 设置云渲染模式
     */
    void setCloudMode(CloudMode mode) { m_cloudMode = mode; }

    /**
     * @brief 获取当前云渲染模式
     */
    [[nodiscard]] CloudMode cloudMode() const { return m_cloudMode; }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief 获取当前云颜色
     */
    [[nodiscard]] const glm::vec4& cloudColor() const { return m_cloudColor; }

private:
    // ========================================================================
    // 资源创建
    // ========================================================================

    /**
     * @brief 创建云顶点缓冲区
     *
     * 生成 Fast 和 Fancy 两种模式的顶点数据。
     * 云网格覆盖 -3 到 +4 区块范围（每个区块 8 格）。
     */
    [[nodiscard]] Result<void> _createCloudVBO();

    /**
     * @brief 创建云纹理
     *
     * 使用程序化生成或加载纹理文件。
     * 云纹理为 256x256 灰度透明度图。
     */
    [[nodiscard]] Result<void> _createTexture(const ResourceManager* resourceManager);

    /**
     * @brief 更新已分配描述符集中的云纹理绑定
     *
     * 在纹理重建后必须调用，否则描述符仍会指向旧图像。
     */
    void _updateTextureDescriptors();

    /**
     * @brief 创建 Uniform 缓冲区
     *
     * 为每帧创建一个 Uniform 缓冲区。
     */
    [[nodiscard]] Result<void> _createUniformBuffers();

    /**
     * @brief 创建描述符集布局
     */
    [[nodiscard]] Result<void> _createDescriptorSetLayout();

    /**
     * @brief 创建描述符池和描述符集
     */
    [[nodiscard]] Result<void> _createDescriptorSets();

    /**
     * @brief 创建管线布局
     */
    [[nodiscard]] Result<void> _createPipelineLayout();

    /**
     * @brief 创建图形管线
     *
     * 创建 Fast 和 Fancy 两种模式的管线。
     */
    [[nodiscard]] Result<void> _createPipelines(VkSampleCountFlagBits sampleCount);

    /**
     * @brief 更新 Uniform 缓冲区
     * @param frameIndex 当前帧索引
     */
    void _updateUniformBuffer(u32 frameIndex);

    /**
     * @brief 更新云网格
     *
     * 当云模式改变时，重新生成顶点数据。
     *
     * @param mode 云渲染模式
     */
    void _updateCloudMesh(CloudMode mode);

    // ========================================================================
    // Vulkan 辅助函数
    // ========================================================================

    /**
     * @brief 查找合适的内存类型
     */
    [[nodiscard]] Result<u32> _findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);

    /**
     * @brief 创建缓冲区
     */
    [[nodiscard]] Result<void> _createBuffer(VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& memory);

    /**
     * @brief 开始单次命令
     */
    VkCommandBuffer _beginSingleTimeCommands();

    /**
     * @brief 结束单次命令
     */
    void _endSingleTimeCommands(VkCommandBuffer commandBuffer);

    /**
     * @brief 生成程序化云纹理数据
     *
     * 使用 Perlin 噪声生成云图案。
     *
     * @param width 纹理宽度
     * @param height 纹理高度
     * @return 纹理像素数据（RGBA）
     */
    [[nodiscard]] std::vector<u8> _generateCloudTexture(u32 width, u32 height);

    /**
     * @brief 从 RGBA 云纹理提取二值云掩码
     *
     * 将 alpha 通道按阈值转换为 0/1 云占据数据，供网格构建时判断。
     *
     * @param textureData RGBA 像素数据
     * @param width 纹理宽度
     * @param height 纹理高度
     */
    void _buildCloudMaskFromTexture(const std::vector<u8>& textureData, u32 width, u32 height);

    /**
     * @brief 查询指定云网格单元是否为"有云"
     *
     * 传入网格坐标（可为负），内部会按纹理尺寸循环映射。
     *
     * @param gridX 云网格 X
     * @param gridZ 云网格 Z
     * @return true 表示该单元存在云体
     */
    [[nodiscard]] bool _isCloudCellOpaque(i32 gridX, i32 gridZ) const;

    // ========================================================================
    // 渲染方法
    // ========================================================================

    /**
     * @brief 渲染 Fast 模式云
     */
    void _renderFast(VkCommandBuffer cmd);

    /**
     * @brief 渲染 Fancy 模式云
     */
    void _renderFancy(VkCommandBuffer cmd);

private:
    // Vulkan 设备
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkExtent2D m_extent = {0, 0};
    bool m_initialized = false;

    // 描述符
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    // 管线
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_fastPipeline = VK_NULL_HANDLE;  // Fast 模式管线
    VkPipeline m_fancyPipeline = VK_NULL_HANDLE; // Fancy 模式管线

    // 顶点缓冲区
    VkBuffer m_fastVBO = VK_NULL_HANDLE; // Fast 模式顶点缓冲区
    VkDeviceMemory m_fastVBOMemory = VK_NULL_HANDLE;
    u32 m_fastVertexCount = 0;

    VkBuffer m_fancyVBO = VK_NULL_HANDLE; // Fancy 模式顶点缓冲区
    VkDeviceMemory m_fancyVBOMemory = VK_NULL_HANDLE;
    u32 m_fancyVertexCount = 0;

    // 纹理
    VkImage m_textureImage = VK_NULL_HANDLE;
    VkDeviceMemory m_textureImageMemory = VK_NULL_HANDLE;
    VkImageView m_textureImageView = VK_NULL_HANDLE;
    VkSampler m_textureSampler = VK_NULL_HANDLE;

    // Uniform 缓冲区（每帧一个）
    static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
    VkBuffer m_uniformBuffers[MAX_FRAMES_IN_FLIGHT] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDeviceMemory m_uniformBuffersMemory[MAX_FRAMES_IN_FLIGHT] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    void* m_uniformBuffersMapped[MAX_FRAMES_IN_FLIGHT] = {nullptr, nullptr};
    VkDescriptorSet m_descriptorSets[MAX_FRAMES_IN_FLIGHT] = {VK_NULL_HANDLE, VK_NULL_HANDLE};

    // 云状态
    CloudMode m_cloudMode = CloudMode::Fancy;
    i64 m_dayTime = 0;
    i64 m_gameTime = 0;
    f64 m_partialTick = 0.0f;
    f64 m_cloudHeight = 192.0f; // 主世界云高度
    glm::vec4 m_cloudColor = glm::vec4(1.0f);
    glm::vec3 m_cameraPos = glm::vec3(0.0f);
    f64 m_cloudOffsetX = 0.0f; // 云 X 偏移（用于动画）

    // 云网格更新检测
    i32 m_cloudsCheckX = 0;       // 上一次检查时的 X 网格坐标
    i32 m_cloudsCheckY = 0;       // 上一次检查时的 Y 网格坐标
    i32 m_cloudsCheckZ = 0;       // 上一次检查时的 Z 网格坐标
    i32 m_cloudGridX = 0;         // 当前 VBO 使用的 X 网格偏移
    i32 m_cloudGridY = 0;         // 当前 VBO 使用的 Y 网格偏移
    i32 m_cloudGridZ = 0;         // 当前 VBO 使用的 Z 网格偏移
    bool m_cloudMeshDirty = true; // 云网格是否需要更新

    // 云纹理二值掩码（按纹理 alpha 提取）
    std::vector<u8> m_cloudMask;
    u32 m_cloudMaskWidth = 0;
    u32 m_cloudMaskHeight = 0;
};

} // namespace mc::client::renderer::trident::cloud
