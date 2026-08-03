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

#include "client/renderer/api/buffer/OffsetAllocatorHeader.hpp" // OffsetAllocator::Allocator / Allocation（带 include guard 包装）
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/Vector4.hpp"
#include <array>
#include <filesystem>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident {
class TridentContext;
}

namespace mc::client::renderer::entity::pipeline {

/**
 * @brief 混合模式枚举
 *
 * 用于不同渲染效果的混合模式。每种模式对应一条专用的 Vulkan 图形管线，
 * 由 _createGraphicsPipeline 创建、bind() 选择、destroy() 销毁。
 *
 * - None：无混合（blendEnable=VK_FALSE），用于不透明/剪切实体渲染。
 *   对应 MC Java 的 withoutBlend()（ENTITY_SOLID / ENTITY_CUTOUT 等）。
 * - Alpha：标准 Alpha 混合（src*srcAlpha + dst*(1-srcAlpha)），默认。
 * - Additive：叠加混合（src*srcAlpha + dst*1），用于眼睛发光、能量光效等。
 * - Multiply：乘法混合（out = 2*src*dst），用于颜色调制/着色叠加。
 *   对应 MC 1.21.11 RenderPipelines.CRUMBLING 的 DST_COLOR/SRC_COLOR 配置，
 *   以及本项目 BreakProgressRenderer 的破坏进度叠加。
 * - Lines：线段渲染（VK_PRIMITIVE_TOPOLOGY_LINE_LIST），用于钓鱼线等。
 */
enum class BlendMode : u8 {
    None,     // 无混合（不透明渲染，blendEnable=VK_FALSE）
    Alpha,    // Alpha 混合（默认）
    Additive, // 叠加混合（用于眼睛发光、能量光效等）
    Multiply, // 乘法混合（out = 2*src*dst，用于颜色调制/着色叠加）
    Lines     // 线段渲染（VK_PRIMITIVE_TOPOLOGY_LINE_LIST，用于钓鱼线等）
};

/**
 * @brief 实体网格数据
 *
 * 不再为每个 mesh 独占 VkBuffer+VkDeviceMemory，而是在统一的 vertex/index
 * mega-buffer 段内子分配一段连续区间。vertexBuffer/indexBuffer 保存所属段的
 * VkBuffer（drawMesh 直接绑定），vertexOffset/indexOffset 为段内偏移。
 * allocation/segmentIndex/alignedSize 供延迟回收时归还给 OffsetAllocator。
 *
 * vertexBuffer == VK_NULL_HANDLE 表示该 mesh 当前未持有任何 GPU 区间（空 mesh）。
 */
struct EntityMesh {
    VkBuffer vertexBuffer = VK_NULL_HANDLE; // 所属 vertex mega-buffer 段的 VkBuffer
    VkBuffer indexBuffer = VK_NULL_HANDLE;  // 所属 index mega-buffer 段的 VkBuffer
    VkDeviceSize vertexOffset = 0;          // vertex 段内偏移（字节）
    VkDeviceSize indexOffset = 0;           // index 段内偏移（字节）
    u32 indexCount = 0;
    u32 vertexCount = 0;

    // 子分配记账（延迟回收时归还给对应段的 OffsetAllocator）
    OffsetAllocator::Allocation vertexAllocation{};
    OffsetAllocator::Allocation indexAllocation{};
    u64 vertexAlignedSize = 0; // 对齐后的 vertex 区间大小（含 padding，free 用）
    u64 indexAlignedSize = 0;  // 对齐后的 index 区间大小
    u32 vertexSegmentIndex = 0;
    u32 indexSegmentIndex = 0;

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
 * - 顶点/索引 mega-buffer 子分配（OffsetAllocator）
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
     * @param context Trident 上下文（用于访问统一暂存缓冲池 stagingPool()）
     * @param device Vulkan逻辑设备
     * @param physicalDevice Vulkan物理设备
     * @param graphicsQueue 图形队列
     * @param renderPass 渲染通道
     * @param cameraDescriptorLayout 相机描述符布局
     * @param descriptorPool 描述符池
     * @param commandPool 命令池（已不再用于单次复制，保留以兼容签名）
     * @param sampleCount MSAA 采样数
     * @param maxFramesInFlight 最大同时在飞帧数（用于 per-frame 描述符集与延迟销毁窗口）
     */
    [[nodiscard]] Result<void> initialize(trident::TridentContext* context,
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkQueue graphicsQueue,
        VkRenderPass renderPass,
        VkDescriptorSetLayout cameraDescriptorLayout,
        VkDescriptorPool descriptorPool,
        VkCommandPool commandPool,
        VkSampleCountFlagBits sampleCount,
        u32 maxFramesInFlight);

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
     * @brief 标记当前帧索引
     *
     * 实体管线为每帧维护独立的纹理描述符集（避免在飞帧读取被本帧 setTextureAtlas
     * 改写的描述符），并在帧边界推进延迟销毁队列。必须在录制本帧实体 draw 之前调用。
     * @param frameIndex 当前帧索引（0 .. maxFramesInFlight-1）
     */
    void beginFrame(u32 frameIndex);

    /**
     * @brief 处理延迟销毁队列，归还足够久未被任何在飞帧引用的子分配区间
     *
     * 必须在每帧 vkQueueSubmit 完成且下一帧 fence 等待之后调用，保证待归还区间
     * 已不再被任何在飞命令缓冲区引用。归还后区间交回 OffsetAllocator 复用，
     * mega-buffer 段本身（VkBuffer/VkDeviceMemory）不销毁。
     */
    void processPendingDestroys();

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
     * @brief 绑定纹理描述符集（set = 1）
     *
     * 绑定当前帧（由 beginFrame 指定）对应的纹理描述符集。每帧独立 set，
     * 避免在飞帧读取被本帧 setTextureAtlas 改写的描述符。
     * @param cmd 命令缓冲区
     */
    void bindTextureDescriptor(VkCommandBuffer cmd);

    /**
     * @brief 设置纹理图集（写入当前帧的纹理描述符集）
     * @param textureView 图集纹理视图
     * @param sampler 采样器
     *
     * 仅更新 beginFrame 指定的当前帧 set，避免改写仍被在飞帧
     * 命令缓冲区引用的描述符。
     */
    void setTextureAtlas(VkImageView textureView, VkSampler sampler);

    /**
     * @brief 将同一纹理图集写入所有帧的纹理描述符集
     *
     * 仅用于初始化阶段（尚无任何帧提交、无在飞命令缓冲区），保证每帧 set
     * 在首次 bindTextureDescriptor 前都指向有效纹理，避免绑定未初始化描述符。
     * 运行时切换图集必须使用 per-frame 的 setTextureAtlas。
     */
    void setTextureAtlasAllFrames(VkImageView textureView, VkSampler sampler);

    /**
     * @brief 获取管线布局
     */
    VkPipelineLayout pipelineLayout() const;

    /**
     * @brief 是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    /// mega-buffer 段：一个大 VkBuffer 内用 OffsetAllocator 子分配
    struct MegaBufferSegment {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize capacity = 0;
        std::unique_ptr<OffsetAllocator::Allocator> allocator;
        u64 localUsedBytes = 0; // 本地记账，用于守恒断言
        u64 localFreeBytes = 0;
    };

    /// 子分配对齐：满足顶点属性访问与 vkCmdBindVertexBuffers 偏移要求
    static constexpr u64 kSubAllocAlign = 16;

    /// 单个 vertex mega-buffer 段容量（32MB）
    static constexpr VkDeviceSize kVertexSegmentCapacity = 32ull * 1024 * 1024;
    /// 单个 index mega-buffer 段容量（8MB）
    static constexpr VkDeviceSize kIndexSegmentCapacity = 8ull * 1024 * 1024;

    trident::TridentContext* m_context = nullptr;
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;              // Alpha 混合管线
    VkPipeline m_additiveBlendPipeline = VK_NULL_HANDLE; // 叠加混合管线
    VkPipeline m_multiplyBlendPipeline = VK_NULL_HANDLE; // 乘法混合管线（DST_COLOR * SRC_COLOR）
    VkPipeline m_noneBlendPipeline = VK_NULL_HANDLE;     // 无混合管线（不透明渲染）
    VkPipeline m_linePipeline = VK_NULL_HANDLE;          // 线段渲染管线（LINE_LIST）
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_textureDescriptorLayout = VK_NULL_HANDLE;
    // 每帧一个纹理描述符集。setTextureAtlas 只写当前帧 set，避免改写仍被在飞帧
    // 命令缓冲区引用的描述符（device-lost 根因之一）。
    std::vector<VkDescriptorSet> m_textureDescriptorSets;
    VkSampler m_textureSampler = VK_NULL_HANDLE;

    u32 m_maxFramesInFlight = 1; // 在飞帧数（延迟销毁保留窗口依据）
    u32 m_currentFrameIndex = 0; // 当前录制帧索引（beginFrame 设置）

    // vertex / index mega-buffer 段集合。OffsetAllocator 不可 resize，容量不足时
    // 追加新段；段 VkBuffer/VkDeviceMemory 仅在 destroy() 释放，子分配区间由
    // OffsetAllocator 复用。多段规避了数据迁移风险。
    std::vector<MegaBufferSegment> m_vertexSegments;
    std::vector<MegaBufferSegment> m_indexSegments;

    // 延迟销毁队列：mesh 的子分配区间不在 destroyMesh 时立即归还，而是入队，
    // 等待足够多帧数（>= maxFramesInFlight + 1）后再归还给对应段的 OffsetAllocator，
    // 保证仍被在飞命令缓冲区引用的区间不被提前复用（device-lost 根因之二）。
    struct PendingDestroy {
        OffsetAllocator::Allocation vertexAllocation{};
        OffsetAllocator::Allocation indexAllocation{};
        u64 vertexAlignedSize = 0;
        u64 indexAlignedSize = 0;
        u32 vertexSegmentIndex = 0;
        u32 indexSegmentIndex = 0;
        bool hasVertex = false;
        bool hasIndex = false;
        u64 enqueueFrame = 0; // 入队时的帧计数器
    };
    std::vector<PendingDestroy> m_pendingDestroys;
    u64 m_frameCounter = 0; // 单调递增帧计数器（processPendingDestroys 推进）

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
     * @brief 创建描述符集（每帧一个纹理描述符集）
     */
    [[nodiscard]] Result<void> _createDescriptorSets();

    /**
     * @brief 将一个 mesh 的子分配区间加入延迟归还队列
     *
     * 不立即 free，而是登记入队帧计数器，由 processPendingDestroys 在足够多帧后
     * 真正归还给 OffsetAllocator。调用后 mesh 的句柄被清零，调用方无需再处理。
     */
    void _enqueueDestroyMesh(EntityMesh& mesh);

    /**
     * @brief 创建图形管线
     */
    [[nodiscard]] Result<void> _createGraphicsPipeline(
        VkRenderPass renderPass, VkDescriptorSetLayout cameraDescriptorLayout, VkSampleCountFlagBits sampleCount);

    /**
     * @brief 在 segments 内子分配一段区间，必要时追加新段
     * @param segments 段集合（vertex 或 index）
     * @param segmentCapacity 单段容量
     * @param alignedSize 需要的对齐大小
     * @param outSegmentIndex 命中段索引
     * @param outAllocation 分配器返回的 Allocation
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> _subAllocate(std::vector<MegaBufferSegment>& segments,
        VkDeviceSize segmentCapacity,
        u64 alignedSize,
        u32& outSegmentIndex,
        OffsetAllocator::Allocation& outAllocation);

    /**
     * @brief 创建一个新 mega-buffer 段（VkBuffer + 内存 + OffsetAllocator）
     */
    [[nodiscard]] Result<void> _createSegment(
        MegaBufferSegment& segment, VkDeviceSize capacity, VkBufferUsageFlags usage);

    /**
     * @brief 归还一段内的子分配区间（守恒断言）
     */
    void _freeAllocation(MegaBufferSegment& segment, const OffsetAllocator::Allocation& alloc, u64 alignedSize);

    /**
     * @brief 通过统一暂存缓冲池上传一段数据到目标 buffer 指定偏移
     *
     * stage → memcpy → copyToBuffer（内部 submit+wait fence）→ release。
     * 目标 buffer 为 mega-buffer 段，dstOffset 为段内子分配偏移。
     */
    [[nodiscard]] Result<void> _uploadViaStagingPool(
        const void* sourceData, u64 size, VkBuffer dstBuffer, VkDeviceSize dstOffset);

    /**
     * @brief 将 size 向上对齐到 kSubAllocAlign
     */
    [[nodiscard]] static u64 _alignUp(u64 size);
};

} // namespace mc::client::renderer::entity::pipeline
