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

#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <filesystem>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace mc::client::renderer::entity::pipeline {

/**
 * @brief 混合模式枚举
 *
 * 用于不同渲染效果的混合模式
 */
enum class BlendMode : u8 {
    None,     // 无混合
    Alpha,    // Alpha 混合（默认）
    Additive, // 叠加混合（用于眼睛发光、能量光效等）
    Multiply, // 乘法混合 // TODO: 尚未实现乘法混合管线，当前回退到Alpha混合
    Lines     // 线段渲染（VK_PRIMITIVE_TOPOLOGY_LINE_LIST，用于钓鱼线等）
};

/**
 * @brief 实体网格数据
 *
 * 存储单个实体的GPU缓冲区
 */
struct EntityMesh {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexMemory = VK_NULL_HANDLE;
    u32 indexCount = 0;
    u32 vertexCount = 0;
    u32 vertexCapacity = 0;
    u32 indexCapacity = 0;

    // 实体位置（用于更新）
    f64 posX = 0.0f;
    f64 posY = 0.0f;
    f64 posZ = 0.0f;
};

/**
 * @brief 实体渲染管线
 *
 * 管理实体渲染的Vulkan资源：
 * - 管线状态
 * - 顶点/索引缓冲区
 * - 描述符集
 * - 纹理图集
 */
class EntityPipeline {
public:
    EntityPipeline();
    ~EntityPipeline();

    // 禁止拷贝
    EntityPipeline(const EntityPipeline&) = delete;
    EntityPipeline& operator=(const EntityPipeline&) = delete;

    /**
     * @brief 初始化管线
     * @param device Vulkan逻辑设备
     * @param physicalDevice Vulkan物理设备
     * @param graphicsQueue 图形队列
     * @param renderPass 渲染通道
     * @param cameraDescriptorLayout 相机描述符布局
     * @param descriptorPool 描述符池
     * @param commandPool 命令池（用于缓冲区复制）
     */
    [[nodiscard]] Result<void> initialize(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkQueue graphicsQueue,
        VkRenderPass renderPass,
        VkDescriptorSetLayout cameraDescriptorLayout,
        VkDescriptorPool descriptorPool,
        VkCommandPool commandPool,
        VkSampleCountFlagBits sampleCount);

    /**
     * @brief 销毁资源
     */
    void destroy();

    /**
     * @brief 绑定管线
     * @param cmd 命令缓冲区
     * @param blendMode 混合模式（默认 Alpha）
     */
    void bind(VkCommandBuffer cmd, BlendMode blendMode = BlendMode::Alpha);

    /**
     * @brief 创建实体网格
     * @param vertices 顶点数据
     * @param indices 索引数据
     * @return 实体网格
     */
    [[nodiscard]] Result<EntityMesh> createMesh(
        const std::vector<model::ModelVertex>& vertices, const std::vector<u32>& indices);

    /**
     * @brief 更新实体网格
     * @param mesh 要更新的网格
     * @param vertices 新顶点数据
     * @param indices 新索引数据
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> updateMesh(
        EntityMesh& mesh, const std::vector<model::ModelVertex>& vertices, const std::vector<u32>& indices);

    /**
     * @brief 销毁实体网格
     * @param mesh 要销毁的网格
     */
    void destroyMesh(EntityMesh& mesh);

    /**
     * @brief 渲染实体网格
     * @param cmd 命令缓冲区
     * @param mesh 网格数据
     * @param modelMatrix 模型矩阵
     * @param position 实体位置
     * @param scale 缩放因子
     * @param overlayColor 覆盖层颜色 (受伤闪烁/经验球色调等)
     * @param hurtTime 受伤时间 (0-10)
     * @param deathTime 死亡时间
     * @param fullbright 全亮光照因子 (0.0=正常光照, 1.0=最大亮度)
     */
    void drawMesh(VkCommandBuffer cmd,
        const EntityMesh& mesh,
        const std::array<f64, 16>& modelMatrix,
        const Vector3f& position,
        f64 scale = 1.0f,
        const Vector4f& overlayColor = Vector4f(0.0f, 0.0f, 0.0f, 0.0f),
        f32 hurtTime = 0.0f,
        f32 deathTime = 0.0f,
        f32 fullbright = 0.0f);

    /**
     * @brief 绑定纹理描述符集
     * @param cmd 命令缓冲区
     */
    void bindTextureDescriptor(VkCommandBuffer cmd);

    /**
     * @brief 设置纹理图集
     * @param texture 图集纹理
     * @param sampler 采样器
     */
    void setTextureAtlas(VkImageView textureView, VkSampler sampler);

    /**
     * @brief 获取管线布局
     */
    VkPipelineLayout pipelineLayout() const;

    /**
     * @brief 是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;              // Alpha 混合管线
    VkPipeline m_additiveBlendPipeline = VK_NULL_HANDLE; // 叠加混合管线
    VkPipeline m_linePipeline = VK_NULL_HANDLE;          // 线段渲染管线（LINE_LIST）
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_textureDescriptorLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_textureDescriptorSet = VK_NULL_HANDLE;
    VkSampler m_textureSampler = VK_NULL_HANDLE;

    bool m_initialized = false;

    /**
     * @brief 获取顶点输入绑定描述
     */
    static VkVertexInputBindingDescription getVertexBindingDescription();

    /**
     * @brief 获取顶点输入属性描述
     */
    static std::vector<VkVertexInputAttributeDescription> getVertexAttributeDescriptions();

    /**
     * @brief 创建描述符布局
     */
    [[nodiscard]] Result<void> _createDescriptorLayouts();

    /**
     * @brief 创建纹理采样器
     */
    [[nodiscard]] Result<void> _createTextureSampler();

    /**
     * @brief 创建描述符集
     */
    [[nodiscard]] Result<void> _createDescriptorSets();

    /**
     * @brief 创建图形管线
     */
    [[nodiscard]] Result<void> _createGraphicsPipeline(
        VkRenderPass renderPass, VkDescriptorSetLayout cameraDescriptorLayout, VkSampleCountFlagBits sampleCount);

    /**
     * @brief 创建缓冲区
     */
    [[nodiscard]] Result<void> _createBuffer(VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& memory);

    /**
     * @brief 确保复用暂存缓冲区容量满足需求
     */
    [[nodiscard]] Result<void> _ensureReusableStagingBuffer(
        VkDeviceSize requiredSize, VkBuffer& buffer, VkDeviceMemory& memory, VkDeviceSize& capacity);

    /**
     * @brief 通过复用暂存缓冲区上传数据到设备本地缓冲区
     */
    [[nodiscard]] Result<void> _uploadToDeviceBuffer(
        const void* sourceData, VkDeviceSize size, VkBuffer destinationBuffer, bool useVertexStagingBuffer);

    /**
     * @brief 销毁复用暂存缓冲区
     */
    void _destroyReusableStagingBuffers();

    /**
     * @brief 复制缓冲区
     */
    void _copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    /**
     * @brief 开始单次命令
     */
    VkCommandBuffer _beginSingleTimeCommands();

    /**
     * @brief 结束单次命令
     */
    void _endSingleTimeCommands(VkCommandBuffer cmd);

    // 单次命令所需的资源
    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    // 复用暂存缓冲区，避免每次网格更新都重新分配 staging memory
    VkBuffer m_vertexStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexStagingMemory = VK_NULL_HANDLE;
    VkDeviceSize m_vertexStagingCapacity = 0;

    VkBuffer m_indexStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexStagingMemory = VK_NULL_HANDLE;
    VkDeviceSize m_indexStagingCapacity = 0;
};

} // namespace mc::client::renderer::entity::pipeline
