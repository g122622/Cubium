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
#include "client/renderer/api/buffer/OffsetAllocatorHeader.hpp" // OffsetAllocator::Allocator / Allocation（带 include guard 包装）
#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/frustum/Frustum.hpp"
#include "common/world/chunk/base/ChunkId.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <glm/ext/vector_double3.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident {
class TridentContext;
}

namespace mc::client {

enum class ChunkRenderLayer : u8 {
    Solid = 0,
    Transparent = 1,
};

// 区块 GPU 缓冲区 —— 不再为每个区块独占 VkBuffer+VkDeviceMemory，而是在统一的
// vertex/index mega-buffer 段内子分配一段连续区间。vertexBuffer/indexBuffer 保存
// 所属段的 VkBuffer（render 直接绑定），vertexOffset/indexOffset 为段内偏移。
// allocation/segmentIndex/alignedSize 供延迟回收时归还给 OffsetAllocator。
//
// vertexBuffer == VK_NULL_HANDLE 表示该区块当前未持有任何 GPU 区间（空网格）。
struct ChunkGpuBuffer {
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

    ChunkId chunkId{0, 0, 0};
    ChunkRenderLayer layer = ChunkRenderLayer::Solid;
    bool isValid = false;
};

// 待回收的子分配区间（延迟归还队列）：旧区间不在 _createChunkBuffer 替换时立即归还，
// 而是入队，等过 framesToKeep 帧（远超 MAX_FRAMES_IN_FLIGHT，确保仍被在飞帧 draw 引用
// 的区间不被提前复用）后由 processPendingDestroys 真正归还给对应段的 OffsetAllocator。
struct PendingBufferDestroy {
    OffsetAllocator::Allocation vertexAllocation{};
    OffsetAllocator::Allocation indexAllocation{};
    u64 vertexAlignedSize = 0;
    u64 indexAlignedSize = 0;
    u32 vertexSegmentIndex = 0;
    u32 indexSegmentIndex = 0;
    bool hasVertex = false;
    bool hasIndex = false;
    u64 frameIndex = 0; // 入队时的帧计数器，用于计算延迟归还
};

// 区块渲染器
class ChunkRenderer {
public:
    ChunkRenderer();
    ~ChunkRenderer();

    // 禁止拷贝
    ChunkRenderer(const ChunkRenderer&) = delete;
    ChunkRenderer& operator=(const ChunkRenderer&) = delete;

    // 初始化
    // @param context Trident 上下文（用于访问统一暂存缓冲池 stagingPool()）
    [[nodiscard]] Result<void> initialize(renderer::trident::TridentContext* context,
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        u32 maxChunks = 8192);

    void destroy();

    // 区块管理
    [[nodiscard]] Result<void> updateChunk(const ChunkId& chunkId, const MeshData& meshData);

    /**
     * @brief 更新区块双层网格
     *
     * @param chunkId 区块 ID
     * @param solidMesh 实心网格
     * @param transparentMesh 半透明网格
     */
    [[nodiscard]] Result<void> updateChunk(
        const ChunkId& chunkId, const MeshData& solidMesh, const MeshData& transparentMesh);

    void removeChunk(const ChunkId& chunkId);

    void clearChunks();

    // 渲染
    void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

    // 渲染（带推送常量回调）
    // pushConstantsCallback: 设置推送常量的回调函数，参数是 chunkId
    using PushConstantsCallback = std::function<void(const ChunkId&)>;
    void render(
        VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, PushConstantsCallback pushConstantsCallback);

    /**
     * @brief 渲染半透明层（可选按距离排序）
     *
     * @param commandBuffer Vulkan 命令缓冲
     * @param pipelineLayout 管线布局
     * @param pushConstantsCallback 区块推送常量回调
     * @param cameraPosition 相机位置（用于排序）
     * @param sortBackToFront 是否按远到近排序
     */
    void renderTransparent(VkCommandBuffer commandBuffer,
        VkPipelineLayout pipelineLayout,
        PushConstantsCallback pushConstantsCallback,
        const glm::dvec3& cameraPosition,
        bool sortBackToFront = true);

    /**
     * @brief 处理延迟归还队列
     *
     * 每帧调用一次，归还不再被任何在飞帧引用的子分配区间给 OffsetAllocator。
     * 应该在帧开始后调用（此时上一轮该 slot 的 GPU 命令已完成）。
     *
     * @param framesToKeep 区间保留帧数（应 >= MAX_FRAMES_IN_FLIGHT，默认 32）
     */
    void processPendingDestroys(u32 framesToKeep = 32);

    // 统计
    u32 chunkCount() const { return static_cast<u32>(m_chunkBuffers.size()); }
    u32 totalVertexCount() const { return m_totalVertices; }
    u32 totalIndexCount() const { return m_totalIndices; }

private:
    [[nodiscard]] static u64 makeBufferKey(const ChunkId& chunkId, ChunkRenderLayer layer)
    {
        return (chunkId.toId() << 1ULL) | static_cast<u64>(layer);
    }

    /// mega-buffer 段：一个大 VkBuffer 内用 OffsetAllocator 子分配
    struct MegaBufferSegment {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize capacity = 0;
        std::unique_ptr<OffsetAllocator::Allocator> allocator;
        u64 localUsedBytes = 0; // 本地记账，用于守恒断言
        u64 localFreeBytes = 0;
    };

    /// 子分配对齐：满足顶点属性访问与 vkCmdBindVertexBuffers/IndexBuffer 偏移要求
    static constexpr u64 kSubAllocAlign = 16;

    /// 单个 vertex mega-buffer 段容量（128MB）
    static constexpr VkDeviceSize kVertexSegmentCapacity = 128ull * 1024 * 1024;
    /// 单个 index mega-buffer 段容量（32MB）
    static constexpr VkDeviceSize kIndexSegmentCapacity = 32ull * 1024 * 1024;

    renderer::trident::TridentContext* m_context = nullptr;
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;

    // 区块缓冲区
    std::unordered_map<u64, std::unique_ptr<ChunkGpuBuffer>> m_chunkBuffers;
    u32 m_maxChunks = 8192;

    // 统计
    u32 m_totalVertices = 0;
    u32 m_totalIndices = 0;

    // vertex / index mega-buffer 段集合。OffsetAllocator 不可 resize，容量不足时
    // 追加新段；段 VkBuffer/VkDeviceMemory 仅在 destroy() 释放，子分配区间由
    // OffsetAllocator 复用。多段规避了数据迁移风险。
    std::vector<MegaBufferSegment> m_vertexSegments;
    std::vector<MegaBufferSegment> m_indexSegments;

    // 延迟归还队列：mesh 替换/移除时旧子分配区间不立即 free，而是入队，等过
    // framesToKeep 帧后再归还给对应段的 OffsetAllocator，保证仍被在飞命令缓冲区引用
    // 的区间不被提前复用（device-lost 根因）。所有调用（updateChunk /
    // processPendingDestroys / render）均在主线程，故无需锁。
    std::vector<PendingBufferDestroy> m_pendingDestroys;
    u64 m_destroyFrameCounter = 0; // 单调递增帧计数器（processPendingDestroys 推进）

    // 创建/更新缓冲区（子分配 + staging 上传）
    [[nodiscard]] Result<void> _createChunkBuffer(ChunkGpuBuffer& buffer, const MeshData& meshData);

    [[nodiscard]] Result<void> _updateChunkLayer(
        const ChunkId& chunkId, const MeshData& meshData, ChunkRenderLayer layer);

    /**
     * @brief 将一个缓冲区的子分配区间加入延迟归还队列
     *
     * 不立即 free，而是登记入队帧计数器，由 processPendingDestroys 在足够多帧后
     * 真正归还给 OffsetAllocator。调用后 buffer 的子分配句柄被清零。
     */
    void _enqueueDestroy(ChunkGpuBuffer& buffer);

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
     * @brief 创建一个新 mega-buffer 段（VkBuffer + 内存 + OffsetAllocator + Tracy 登记）
     */
    [[nodiscard]] Result<void> _createSegment(
        MegaBufferSegment& segment, VkDeviceSize capacity, VkBufferUsageFlags usage, const char* tracyPoolName);

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

} // namespace mc::client
