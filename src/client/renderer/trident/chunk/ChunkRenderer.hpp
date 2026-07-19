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
#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/frustum/Frustum.hpp"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>

namespace mc::client {

enum class ChunkRenderLayer : u8 {
    Solid = 0,
    Transparent = 1,
};

// 区块GPU缓冲区 - 使用原始 Vulkan handles
struct ChunkGpuBuffer {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexMemory = VK_NULL_HANDLE;
    u32 indexCount = 0;
    u32 vertexCount = 0;
    // 缓冲区容量（请求 size，非 vkGetBufferMemoryRequirements 对齐后的 size）。
    // 用于 free-list 容量池判断新数据能否复用本 buffer：newVertexSize <= vertexCapacity 即可。
    VkDeviceSize vertexCapacity = 0;
    VkDeviceSize indexCapacity = 0;
    ChunkId chunkId{0, 0, 0};
    ChunkRenderLayer layer = ChunkRenderLayer::Solid;
    bool isValid = false;

    void destroy(VkDevice device);
};

// 待销毁的缓冲区（用于延迟销毁）
struct PendingBufferDestroy {
    std::unique_ptr<ChunkGpuBuffer> buffer;
    u64 frameIndex; // 创建时的帧号，用于计算延迟销毁
};

// Fence 管理器（用于非阻塞上传）
struct FenceManager {
    std::vector<VkFence> fences;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<bool> inUse;
    u32 nextIndex = 0;

    void cleanup(VkDevice device, VkCommandPool commandPool);
    void destroy(VkDevice device, VkCommandPool commandPool);
};

// 区块渲染器
class ChunkRenderer {
public:
    struct StagingCopyLayout {
        VkDeviceSize vertexOffset = 0;
        VkDeviceSize indexOffset = 0;
        VkDeviceSize totalSize = 0;
    };

    [[nodiscard]] static constexpr VkDeviceSize stagingCopyAlignment() { return 4; }

    [[nodiscard]] static constexpr VkDeviceSize alignStagingOffset(VkDeviceSize value, VkDeviceSize alignment)
    {
        return alignment == 0 ? value : ((value + alignment - 1) / alignment) * alignment;
    }

    [[nodiscard]] static constexpr StagingCopyLayout buildStagingCopyLayout(
        VkDeviceSize vertexSize, VkDeviceSize indexSize)
    {
        StagingCopyLayout layout{};
        layout.vertexOffset = 0;
        layout.indexOffset = alignStagingOffset(vertexSize, stagingCopyAlignment());
        layout.totalSize = layout.indexOffset + indexSize;
        return layout;
    }

    ChunkRenderer();
    ~ChunkRenderer();

    // 禁止拷贝
    ChunkRenderer(const ChunkRenderer&) = delete;
    ChunkRenderer& operator=(const ChunkRenderer&) = delete;

    // 初始化
    [[nodiscard]] Result<void> initialize(VkDevice device,
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
     * @brief 处理延迟销毁队列
     *
     * 每帧调用一次，销毁不再使用的 GPU 缓冲区。
     * 应该在帧开始后调用，因为此时 GPU 命令已完成。
     *
     * @param framesToKeep 缓冲区保留帧数（应该 >= MAX_FRAMES_IN_FLIGHT，默认 3）
     */
    void processPendingDestroys(u32 framesToKeep = 3);

    // 统计
    u32 chunkCount() const { return static_cast<u32>(m_chunkBuffers.size()); }
    u32 totalVertexCount() const { return m_totalVertices; }
    u32 totalIndexCount() const { return m_totalIndices; }

private:
    [[nodiscard]] static u64 makeBufferKey(const ChunkId& chunkId, ChunkRenderLayer layer)
    {
        return (chunkId.toId() << 1ULL) | static_cast<u64>(layer);
    }

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

    // 暂存缓冲区
    VkBuffer m_stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_stagingMemory = VK_NULL_HANDLE;
    VkDeviceSize m_stagingBufferSize = 16 * 1024 * 1024; // 16MB
    void* m_stagingMapped = nullptr;

    // 延迟销毁队列：旧 buffer 先在此冷却 framesToKeep 帧（等待在飞帧 draw 完成），
    // 到期后转入 m_freeList 供复用，而非直接销毁。
    // 用 recursive_mutex：processPendingDestroys 持锁调 _releaseToFreeList，后者也需加锁保护
    // m_freeList，故锁须可重入。free-list 与 pending 共用同一把锁，避免新增锁。
    std::vector<PendingBufferDestroy> m_pendingDestroys;
    mutable std::recursive_mutex m_pendingDestroysMutex;
    u64 m_destroyFrameCounter = 0; // 每次调用 processPendingDestroys 递增

    // free-list 容量池：冷却完成的 buffer 按容量入池，供后续 _createChunkBuffer 复用整 buffer
    // （vertex+index 一起），避免每次 updateChunk 都 vkAllocateMemory 新建、峰期新旧 buffer 共存翻倍。
    // 入池时机 = processPendingDestroys 中 frameDiff >= framesToKeep 判定点，此时已无在飞帧引用。
    // Tracy 追踪权随句柄跨对象转移（活跃→pending→free-list→复用活跃），每个阶段 free 旧地址 + alloc 新地址。
    struct FreeListEntry {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexMemory = VK_NULL_HANDLE;
        VkDeviceSize vertexCapacity = 0;
        VkDeviceSize indexCapacity = 0;
        u64 enlistSeq = 0; // 入池序号，FIFO 淘汰用
    };
    // key = vertexCapacity，multimap 允许同容量多 buffer；lower_bound 实现 best-fit。
    std::multimap<VkDeviceSize, FreeListEntry> m_freeList;
    u64 m_freeListEnlistCounter = 0;
    VkDeviceSize m_freeListTotalBytes = 0;
    // free-list 上限：超限时按 FIFO 真销毁最旧条目，避免显存无限占用。
    static constexpr u32 kFreeListMaxCount = 64;
    static constexpr VkDeviceSize kFreeListMaxBytes = 32 * 1024 * 1024; // 32MB

    // 单次命令缓冲区
    [[nodiscard]] Result<VkCommandBuffer> _beginSingleTimeCommands();
    void _endSingleTimeCommands(VkCommandBuffer commandBuffer);

    // 创建缓冲区
    [[nodiscard]] Result<void> _createBuffer(VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& memory);

    // 创建/更新缓冲区
    [[nodiscard]] Result<void> _createChunkBuffer(ChunkGpuBuffer& buffer, const MeshData& meshData);

    [[nodiscard]] Result<void> _updateChunkLayer(
        const ChunkId& chunkId, const MeshData& meshData, ChunkRenderLayer layer);

    // 从 free-list 获取容量足够的整 buffer（vertex+index 一起复用）。best-fit 查找。
    // 命中时把句柄和 capacity 写入 out，并完成 Tracy 追踪权转移（entry 地址 → out 地址）。
    // 未命中返回 false，调用方走 _createBuffer 新建路径。须在 m_pendingDestroysMutex 保护下调用。
    [[nodiscard]] bool _acquireFromFreeList(VkDeviceSize vertexSize, VkDeviceSize indexSize, ChunkGpuBuffer& out);

    // 把冷却完成的 buffer 句柄转入 free-list。超限时按 FIFO 真销毁最旧条目。
    // 调用方须先 free 旧持有地址的 Tracy alloc，本函数在 entry 新地址上重新 trackGpuAlloc。
    // 须在 m_pendingDestroysMutex 保护下调用。
    void _releaseToFreeList(VkBuffer vertexBuffer,
        VkDeviceMemory vertexMemory,
        VkBuffer indexBuffer,
        VkDeviceMemory indexMemory,
        VkDeviceSize vertexCapacity,
        VkDeviceSize indexCapacity);

    // 真正销毁 free-list entry 的 GPU 资源并配对 Tracy free。须在 m_pendingDestroysMutex 保护下调用。
    void _destroyFreeListEntry(FreeListEntry& entry);

    // 销毁整个 free-list（destroy 时调用）。
    void _destroyFreeList();

    // 上传缓冲区数据
    [[nodiscard]] Result<void> _uploadBufferData(VkBuffer dstBuffer, const void* data, VkDeviceSize size);

    // 查找内存类型
    [[nodiscard]] Result<u32> _findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
};

} // namespace mc::client
