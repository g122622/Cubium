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

#include "ChunkRenderer.hpp"
#include "client/renderer/trident/core/TridentContext.hpp"
#include "client/renderer/trident/util/VulkanUtils.hpp"
#include "common/core/Constants.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <algorithm>
#include <cstring>
#include <glm/geometric.hpp>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::client {

// ============================================================================
// GPU 内存追踪辅助
// ============================================================================
//
// Tracy GPU 显存追踪现在按 mega-buffer 段级登记（每段一次 vkAllocateMemory）。
// 段的 &segment.memory 地址在段生命周期内稳定，alloc（_createSegment）与
// free（destroy 段销毁）严格一对一。子分配级不再单独追踪——OffsetAllocator 的
// 守恒断言（_freeAllocation 内 storageReport().totalFreeSpace == localFreeBytes）
// 已覆盖子分配正确性，Tracy 的二级堆追踪 API 不适用于 GPU 子分配。
//
// Tracy 按 name 指针区分内存池（见 tracy 手册 3.1.2 节），故 name 必须是同一字面量
// 的稳定指针。下面用文件作用域 const char* const 变量持有，保证 alloc/free 传入同一指针。

namespace {

/// 顶点缓冲区 GPU 内存池名
const char* const kGpuPoolChunkVtx = "ChunkVtx";
/// 索引缓冲区 GPU 内存池名
const char* const kGpuPoolChunkIdx = "ChunkIdx";

/// 登记一次 GPU 内存分配。memPtr 指向 MegaBufferSegment 中存储 VkDeviceMemory 句柄的成员地址。
void trackGpuAlloc(VkDeviceMemory* memPtr, VkDeviceSize size, const char* name)
{
    MC_TRACE_MEM_ALLOC(name, memPtr, size);
}

/// 登记一次 GPU 内存释放。调用方须保证 memPtr 对应的 alloc 已发（即 *memPtr != VK_NULL_HANDLE）。
void trackGpuFree(VkDeviceMemory* memPtr, const char* name)
{
    MC_TRACE_MEM_FREE(name, memPtr);
}

} // namespace

// ============================================================================
// ChunkRenderer 实现
// ============================================================================

ChunkRenderer::ChunkRenderer() = default;

ChunkRenderer::~ChunkRenderer()
{
    destroy();
}

Result<void> ChunkRenderer::initialize(renderer::trident::TridentContext* context,
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    u32 maxChunks)
{
    m_context = context;
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    // 1024 在高视距或双层网格场景下容易触顶，导致近处区块上传失败。
    m_maxChunks = std::max(maxChunks, 8192u);

    spdlog::info(
        "ChunkRenderer initialized (requested max chunks: {}, effective max chunks: {})", maxChunks, m_maxChunks);
    return {};
}

void ChunkRenderer::destroy()
{
    // 关闭阶段：强制归还所有仍在延迟队列中的子分配区间（绕过帧延迟窗口）。
    // 此刻设备即将销毁，延迟窗口已无意义，但守恒断言要求段内 localUsedBytes 归零，
    // 故先归还再销毁 VkBuffer。注意：仍存在"所有者从未触发回收的 live 区间"
    // （_createChunkBuffer 替换时旧区间已入队，但 removeChunk/clearChunks 移除后若未
    // 过 framesToKeep 帧就在此 destroy）——其区间在此归还。
    // 同时把所有仍活跃的区块缓冲区区间也入队并立即归还（TridentEngine::destroy 直接
    // 调 destroy 而非 clearChunks，活跃区块的区间需在此一并归还，避免关闭期泄漏告警）。
    for (auto& pair : m_chunkBuffers) {
        if (pair.second) {
            _enqueueDestroy(*pair.second);
        }
    }
    m_chunkBuffers.clear();
    m_totalVertices = 0;
    m_totalIndices = 0;

    for (const auto& pending : m_pendingDestroys) {
        if (pending.hasVertex && pending.vertexSegmentIndex < m_vertexSegments.size()) {
            _freeAllocation(
                m_vertexSegments[pending.vertexSegmentIndex], pending.vertexAllocation, pending.vertexAlignedSize);
        }
        if (pending.hasIndex && pending.indexSegmentIndex < m_indexSegments.size()) {
            _freeAllocation(
                m_indexSegments[pending.indexSegmentIndex], pending.indexAllocation, pending.indexAlignedSize);
        }
    }
    m_pendingDestroys.clear();

    // 关闭阶段清理：所有子分配区间随段 VkBuffer 一并销毁，无需再归还给 OffsetAllocator。
    // 若某段仍有 localUsedBytes != 0，说明存在所有者在关闭时未触发回收的 live 区间
    // （如进程退出时区块仍活跃）。设备已 idle 且整段 VkBuffer 即将销毁，此为关闭期
    // 可接受的丢弃；记录泄漏量但不视为致命错误（运行期的真实泄漏由 _freeAllocation
    // 的守恒断言在渲染期间捕获）。
    auto destroySegments = [this](std::vector<MegaBufferSegment>& segments, const char* tracyPoolName) {
        for (auto& seg : segments) {
            if (seg.localUsedBytes != 0) {
                spdlog::warn("ChunkRenderer: {} mega-buffer segment leaked {} bytes at shutdown "
                             "(chunk owner did not recycle before renderer teardown)",
                    tracyPoolName,
                    seg.localUsedBytes);
            }
            if (seg.memory != VK_NULL_HANDLE) {
                // 与 _createSegment 中的 alloc 严格一对一（同一 &seg.memory 地址）。
                trackGpuFree(&seg.memory, tracyPoolName);
            }
            if (seg.buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(m_device, seg.buffer, nullptr);
            }
            if (seg.memory != VK_NULL_HANDLE) {
                vkFreeMemory(m_device, seg.memory, nullptr);
            }
            seg.allocator.reset();
        }
        segments.clear();
    };
    destroySegments(m_vertexSegments, kGpuPoolChunkVtx);
    destroySegments(m_indexSegments, kGpuPoolChunkIdx);

    m_context = nullptr;
    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_graphicsQueue = VK_NULL_HANDLE;
    m_destroyFrameCounter = 0;
}

Result<void> ChunkRenderer::updateChunk(const ChunkId& chunkId, const MeshData& meshData)
{
    return _updateChunkLayer(chunkId, meshData, ChunkRenderLayer::Solid);
}

Result<void> ChunkRenderer::updateChunk(
    const ChunkId& chunkId, const MeshData& solidMesh, const MeshData& transparentMesh)
{
    auto solidResult = _updateChunkLayer(chunkId, solidMesh, ChunkRenderLayer::Solid);
    if (solidResult.failed()) {
        return solidResult;
    }

    return _updateChunkLayer(chunkId, transparentMesh, ChunkRenderLayer::Transparent);
}

Result<void> ChunkRenderer::_updateChunkLayer(const ChunkId& chunkId, const MeshData& meshData, ChunkRenderLayer layer)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame,
        "UpdateChunkLayer",
        "layer",
        static_cast<int>(layer),
        [flow = ::perfetto::Flow::ProcessScoped(ChunkPos(chunkId.x, chunkId.z).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    const u64 id = makeBufferKey(chunkId, layer);

    if (meshData.empty()) {
        auto it = m_chunkBuffers.find(id);
        if (it != m_chunkBuffers.end()) {
            m_totalVertices -= it->second->vertexCount;
            m_totalIndices -= it->second->indexCount;

            // 旧子分配区间延迟归还（仍可能被在飞命令缓冲区引用）
            _enqueueDestroy(*it->second);
            m_chunkBuffers.erase(it);
        }
        return {};
    }

    // 查找或创建缓冲区
    auto it = m_chunkBuffers.find(id);

    if (it == m_chunkBuffers.end()) {
        if (m_chunkBuffers.size() >= m_maxChunks) {
            return Error(ErrorCode::CapacityExceeded, "Maximum chunk count reached");
        }

        auto buffer = std::make_unique<ChunkGpuBuffer>();
        buffer->chunkId = chunkId;
        buffer->layer = layer;

        auto result = _createChunkBuffer(*buffer, meshData);
        if (!result.success()) {
            return result;
        }

        m_chunkBuffers[id] = std::move(buffer);
    } else {
        // 更新现有缓冲区
        auto result = _createChunkBuffer(*it->second, meshData);
        if (!result.success()) {
            return result;
        }
    }

    return {};
}

void ChunkRenderer::removeChunk(const ChunkId& chunkId)
{
    const std::array<ChunkRenderLayer, 2> layers = {ChunkRenderLayer::Solid, ChunkRenderLayer::Transparent};

    for (const auto layer : layers) {
        const u64 id = makeBufferKey(chunkId, layer);
        auto it = m_chunkBuffers.find(id);
        if (it == m_chunkBuffers.end()) {
            continue;
        }

        m_totalVertices -= it->second->vertexCount;
        m_totalIndices -= it->second->indexCount;

        _enqueueDestroy(*it->second);
        m_chunkBuffers.erase(it);
    }
}

void ChunkRenderer::clearChunks()
{
    // 将所有缓冲区的子分配区间移入延迟归还队列
    for (auto& pair : m_chunkBuffers) {
        if (pair.second && pair.second->isValid) {
            _enqueueDestroy(*pair.second);
        }
    }

    m_chunkBuffers.clear();
    m_totalVertices = 0;
    m_totalIndices = 0;
}

Result<void> ChunkRenderer::_createChunkBuffer(ChunkGpuBuffer& buffer, const MeshData& meshData)
{
    for (u16 index : meshData.indices) {
        if (index >= meshData.vertices.size()) {
            return Error(ErrorCode::InvalidData,
                "Chunk mesh index out of range for chunk (" + std::to_string(buffer.chunkId.x) + ", " +
                    std::to_string(buffer.chunkId.z) + ")");
        }
    }

    const u32 oldVertexCount = buffer.vertexCount;
    const u32 oldIndexCount = buffer.indexCount;

    const u64 vertexUploadSize = sizeof(Vertex) * meshData.vertices.size();
    const u64 indexUploadSize = sizeof(u16) * meshData.indices.size();
    const u64 vertexAlignedSize = _alignUp(vertexUploadSize);
    const u64 indexAlignedSize = _alignUp(indexUploadSize);

    // 旧子分配区间延迟归还（仍可能被在飞命令缓冲区引用）。
    // mega-buffer 子分配大小在分配时固定，updateChunk 每次都重新子分配新区间
    // （不复用旧区间，避免原地覆写与在飞帧并发访问的 device lost 风险），旧区间
    // 入队等过 framesToKeep 帧后由 processPendingDestroys 归还给 OffsetAllocator 复用。
    if (buffer.vertexBuffer != VK_NULL_HANDLE || buffer.indexBuffer != VK_NULL_HANDLE) {
        _enqueueDestroy(buffer);
    }

    // vertex mega-buffer 段子分配
    auto vtxResult = _subAllocate(m_vertexSegments,
        kVertexSegmentCapacity,
        vertexAlignedSize,
        buffer.vertexSegmentIndex,
        buffer.vertexAllocation);
    if (!vtxResult.success()) {
        buffer = {};
        return vtxResult.error();
    }
    buffer.vertexBuffer = m_vertexSegments[buffer.vertexSegmentIndex].buffer;
    buffer.vertexOffset = buffer.vertexAllocation.offset;
    buffer.vertexAlignedSize = vertexAlignedSize;

    auto uploadResult =
        _uploadViaStagingPool(meshData.vertices.data(), vertexUploadSize, buffer.vertexBuffer, buffer.vertexOffset);
    if (!uploadResult.success()) {
        // vertex 区间已子分配，但上传失败（staging OOM 或 submit 失败）。submitAsyncCopy
        // 失败时已自行 release staging 句柄，无在飞 copy；但若 stage 成功后 submit 链路某步
        // 失败已留下在飞命令，waitIdle 兜底确保复制完成后才归还 mega-buffer 区间，避免
        // 归还的区间被异步 copy 覆写。
        if (m_context) {
            m_context->waitIdle();
            if (auto* pool = m_context->stagingPool()) {
                pool->pollAsyncCopies();
            }
        }
        _freeAllocation(m_vertexSegments[buffer.vertexSegmentIndex], buffer.vertexAllocation, vertexAlignedSize);
        buffer = {};
        return uploadResult.error();
    }

    // index mega-buffer 段子分配
    auto idxResult = _subAllocate(
        m_indexSegments, kIndexSegmentCapacity, indexAlignedSize, buffer.indexSegmentIndex, buffer.indexAllocation);
    if (!idxResult.success()) {
        // vertex 上传已异步 submit（在飞），归还其 mega-buffer 区间前须等 copy 完成。
        if (m_context) {
            m_context->waitIdle();
            if (auto* pool = m_context->stagingPool()) {
                pool->pollAsyncCopies();
            }
        }
        _freeAllocation(m_vertexSegments[buffer.vertexSegmentIndex], buffer.vertexAllocation, vertexAlignedSize);
        buffer = {};
        return idxResult.error();
    }
    buffer.indexBuffer = m_indexSegments[buffer.indexSegmentIndex].buffer;
    buffer.indexOffset = buffer.indexAllocation.offset;
    buffer.indexAlignedSize = indexAlignedSize;

    uploadResult =
        _uploadViaStagingPool(meshData.indices.data(), indexUploadSize, buffer.indexBuffer, buffer.indexOffset);
    if (!uploadResult.success()) {
        // vertex/index 区间均有在飞 copy，waitIdle 兜底后再归还。
        if (m_context) {
            m_context->waitIdle();
            if (auto* pool = m_context->stagingPool()) {
                pool->pollAsyncCopies();
            }
        }
        _freeAllocation(m_vertexSegments[buffer.vertexSegmentIndex], buffer.vertexAllocation, vertexAlignedSize);
        _freeAllocation(m_indexSegments[buffer.indexSegmentIndex], buffer.indexAllocation, indexAlignedSize);
        buffer = {};
        return uploadResult.error();
    }

    buffer.indexCount = static_cast<u32>(meshData.indices.size());
    buffer.vertexCount = static_cast<u32>(meshData.vertices.size());
    buffer.isValid = true;

    // 更新统计
    MC_ASSERT_RELEASE(m_totalVertices >= oldVertexCount);
    m_totalVertices -= oldVertexCount;
    MC_ASSERT_RELEASE(m_totalIndices >= oldIndexCount);
    m_totalIndices -= oldIndexCount;

    m_totalVertices += buffer.vertexCount;
    m_totalIndices += buffer.indexCount;

    return {};
}

void ChunkRenderer::_enqueueDestroy(ChunkGpuBuffer& buffer)
{
    PendingBufferDestroy pending;
    if (buffer.vertexBuffer != VK_NULL_HANDLE) {
        pending.vertexAllocation = buffer.vertexAllocation;
        pending.vertexAlignedSize = buffer.vertexAlignedSize;
        pending.vertexSegmentIndex = buffer.vertexSegmentIndex;
        pending.hasVertex = true;
    }
    if (buffer.indexBuffer != VK_NULL_HANDLE) {
        pending.indexAllocation = buffer.indexAllocation;
        pending.indexAlignedSize = buffer.indexAlignedSize;
        pending.indexSegmentIndex = buffer.indexSegmentIndex;
        pending.hasIndex = true;
    }
    pending.frameIndex = m_destroyFrameCounter;
    m_pendingDestroys.push_back(std::move(pending));

    buffer.vertexBuffer = VK_NULL_HANDLE;
    buffer.indexBuffer = VK_NULL_HANDLE;
    buffer.vertexOffset = 0;
    buffer.indexOffset = 0;
    buffer.vertexAllocation = {};
    buffer.indexAllocation = {};
    buffer.vertexAlignedSize = 0;
    buffer.indexAlignedSize = 0;
    buffer.vertexSegmentIndex = 0;
    buffer.indexSegmentIndex = 0;
    buffer.isValid = false;
}

void ChunkRenderer::render(VkCommandBuffer commandBuffer, VkPipelineLayout /*pipelineLayout*/)
{
    // 绑定纹理（如果有效）
    // 注意: 实际的描述符绑定需要在外部处理

    // 渲染所有区块
    for (const auto& pair : m_chunkBuffers) {
        const auto& buffer = pair.second;
        if (buffer->layer != ChunkRenderLayer::Solid) {
            continue;
        }
        if (!buffer->isValid || buffer->vertexBuffer == VK_NULL_HANDLE || buffer->indexBuffer == VK_NULL_HANDLE) {
            continue;
        }

        VkBuffer vertexBuffers[] = {buffer->vertexBuffer};
        VkDeviceSize offsets[] = {buffer->vertexOffset};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, buffer->indexBuffer, buffer->indexOffset, VK_INDEX_TYPE_UINT16);

        vkCmdDrawIndexed(commandBuffer,
            buffer->indexCount,
            1, // instance count
            0, // first index
            0, // vertex offset
            0  // first instance
        );
    }
}

void ChunkRenderer::render(
    VkCommandBuffer commandBuffer, VkPipelineLayout /*pipelineLayout*/, PushConstantsCallback pushConstantsCallback)
{
    // 渲染所有区块
    for (const auto& pair : m_chunkBuffers) {
        const auto& buffer = pair.second;
        if (buffer->layer != ChunkRenderLayer::Solid) {
            continue;
        }
        if (!buffer->isValid || buffer->vertexBuffer == VK_NULL_HANDLE || buffer->indexBuffer == VK_NULL_HANDLE) {
            continue;
        }

        // 调用回调设置推送常量（区块偏移）
        if (pushConstantsCallback) {
            pushConstantsCallback(buffer->chunkId);
        }

        VkBuffer vertexBuffers[] = {buffer->vertexBuffer};
        VkDeviceSize offsets[] = {buffer->vertexOffset};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, buffer->indexBuffer, buffer->indexOffset, VK_INDEX_TYPE_UINT16);

        vkCmdDrawIndexed(commandBuffer,
            buffer->indexCount,
            1, // instance count
            0, // first index
            0, // vertex offset
            0  // first instance
        );
    }
}

void ChunkRenderer::renderTransparent(VkCommandBuffer commandBuffer,
    VkPipelineLayout /*pipelineLayout*/,
    PushConstantsCallback pushConstantsCallback,
    const glm::dvec3& cameraPosition,
    bool sortBackToFront)
{
    std::vector<const ChunkGpuBuffer*> transparentBuffers;
    transparentBuffers.reserve(m_chunkBuffers.size());

    for (const auto& pair : m_chunkBuffers) {
        const auto& buffer = pair.second;
        if (buffer->layer != ChunkRenderLayer::Transparent) {
            continue;
        }
        if (!buffer->isValid || buffer->vertexBuffer == VK_NULL_HANDLE || buffer->indexBuffer == VK_NULL_HANDLE) {
            continue;
        }
        transparentBuffers.push_back(buffer.get());
    }

    if (sortBackToFront) {
        std::sort(transparentBuffers.begin(),
            transparentBuffers.end(),
            [&cameraPosition](const ChunkGpuBuffer* lhs, const ChunkGpuBuffer* rhs) {
                const glm::dvec3 lhsCenter(
                    static_cast<f64>(lhs->chunkId.x * world::CHUNK_WIDTH + world::CHUNK_WIDTH / 2),
                    static_cast<f64>(world::SEA_LEVEL),
                    static_cast<f64>(lhs->chunkId.z * world::CHUNK_WIDTH + world::CHUNK_WIDTH / 2));
                const glm::dvec3 rhsCenter(
                    static_cast<f64>(rhs->chunkId.x * world::CHUNK_WIDTH + world::CHUNK_WIDTH / 2),
                    static_cast<f64>(world::SEA_LEVEL),
                    static_cast<f64>(rhs->chunkId.z * world::CHUNK_WIDTH + world::CHUNK_WIDTH / 2));

                const f64 lhsDist2 = glm::dot(lhsCenter - cameraPosition, lhsCenter - cameraPosition);
                const f64 rhsDist2 = glm::dot(rhsCenter - cameraPosition, rhsCenter - cameraPosition);
                return lhsDist2 > rhsDist2;
            });
    }

    for (const ChunkGpuBuffer* buffer : transparentBuffers) {
        if (pushConstantsCallback) {
            pushConstantsCallback(buffer->chunkId);
        }

        VkBuffer vertexBuffers[] = {buffer->vertexBuffer};
        VkDeviceSize offsets[] = {buffer->vertexOffset};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, buffer->indexBuffer, buffer->indexOffset, VK_INDEX_TYPE_UINT16);

        vkCmdDrawIndexed(commandBuffer, buffer->indexCount, 1, 0, 0, 0);
    }
}

void ChunkRenderer::processPendingDestroys(u32 framesToKeep)
{
    if (m_pendingDestroys.empty()) {
        // 即便队列为空也推进帧计数器，保证后续入队条目的 age 计算正确。
        ++m_destroyFrameCounter;
        return;
    }

    const u64 currentCounter = m_destroyFrameCounter++;

    // 归还已冷却足够帧数的子分配区间。此时已过 framesToKeep 帧（远超 MAX_FRAMES_IN_FLIGHT），
    // 在飞帧 draw 早已完成，区间可安全归还给 OffsetAllocator 复用。VkBuffer 段本身不销毁。
    for (auto it = m_pendingDestroys.begin(); it != m_pendingDestroys.end();) {
        const u64 age = (currentCounter >= it->frameIndex) ? (currentCounter - it->frameIndex) : 0;

        if (age >= framesToKeep) {
            if (it->hasVertex && it->vertexSegmentIndex < m_vertexSegments.size()) {
                _freeAllocation(m_vertexSegments[it->vertexSegmentIndex], it->vertexAllocation, it->vertexAlignedSize);
            }
            if (it->hasIndex && it->indexSegmentIndex < m_indexSegments.size()) {
                _freeAllocation(m_indexSegments[it->indexSegmentIndex], it->indexAllocation, it->indexAlignedSize);
            }
            it = m_pendingDestroys.erase(it);
        } else {
            ++it;
        }
    }
}

u64 ChunkRenderer::_alignUp(u64 size)
{
    return (size + kSubAllocAlign - 1) & ~(kSubAllocAlign - 1);
}

Result<void> ChunkRenderer::_createSegment(
    MegaBufferSegment& segment, VkDeviceSize capacity, VkBufferUsageFlags usage, const char* tracyPoolName)
{
    segment.capacity = capacity;
    auto result = renderer::VulkanUtils::createBuffer(m_device,
        m_physicalDevice,
        capacity,
        usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        segment.buffer,
        segment.memory);
    if (!result.success()) {
        return result.error();
    }

    // GPU 内存追踪：段级登记（每段一次 vkAllocateMemory，与 destroy 中段销毁的 free 一对一）。
    trackGpuAlloc(&segment.memory, capacity, tracyPoolName);

    // OffsetAllocator 容量上限为 u32；段容量 128MB/32MB 远在范围内。
    segment.allocator = std::make_unique<OffsetAllocator::Allocator>(static_cast<u32>(capacity));
    segment.localUsedBytes = 0;
    segment.localFreeBytes = capacity;
    return Result<void>::ok();
}

Result<void> ChunkRenderer::_subAllocate(std::vector<MegaBufferSegment>& segments,
    VkDeviceSize segmentCapacity,
    u64 alignedSize,
    u32& outSegmentIndex,
    OffsetAllocator::Allocation& outAllocation)
{
    if (alignedSize == 0) {
        return Error(ErrorCode::InvalidArgument, "ChunkRenderer::_subAllocate: zero size");
    }

    // 在已有段中寻找可容纳的空闲区间
    // 注：OffsetAllocator::allocate 仅接受 size，无 align 参数。因所有请求经 _alignUp
    // 向上取整到 kSubAllocAlign=16，且段从对齐的偏移 0 起按 16 倍数切分，
    // 返回 offset 保持 16 倍数对齐（满足顶点/索引绑定偏移要求）。
    for (u32 i = 0; i < segments.size(); ++i) {
        auto& seg = segments[i];
        OffsetAllocator::Allocation alloc = seg.allocator->allocate(static_cast<u32>(alignedSize));
        if (alloc.offset != OffsetAllocator::Allocation::NO_SPACE) {
            outSegmentIndex = i;
            outAllocation = alloc;
            seg.localUsedBytes += alignedSize;
            seg.localFreeBytes -= alignedSize;
            return Result<void>::ok();
        }
    }

    // 所有段均无足够连续空间，追加新段
    // 若单次请求超过段容量，按对齐大小向上取整作为新段容量（罕见，如巨型区块网格）
    const VkDeviceSize newCapacity = std::max<VkDeviceSize>(segmentCapacity, alignedSize);
    MegaBufferSegment newSeg;
    const VkBufferUsageFlags usage =
        (&segments == &m_vertexSegments) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    const char* poolName = (&segments == &m_vertexSegments) ? kGpuPoolChunkVtx : kGpuPoolChunkIdx;
    auto result = _createSegment(newSeg, newCapacity, usage, poolName);
    if (!result.success()) {
        return result.error();
    }
    segments.push_back(std::move(newSeg));
    auto& seg = segments.back();

    OffsetAllocator::Allocation alloc = seg.allocator->allocate(static_cast<u32>(alignedSize));
    if (alloc.offset == OffsetAllocator::Allocation::NO_SPACE) {
        return Error(ErrorCode::OutOfMemory, "ChunkRenderer::_subAllocate: fresh segment rejected allocation");
    }
    outSegmentIndex = static_cast<u32>(segments.size() - 1);
    outAllocation = alloc;
    seg.localUsedBytes += alignedSize;
    seg.localFreeBytes -= alignedSize;
    return Result<void>::ok();
}

void ChunkRenderer::_freeAllocation(
    MegaBufferSegment& segment, const OffsetAllocator::Allocation& alloc, u64 alignedSize)
{
    if (alloc.offset == OffsetAllocator::Allocation::NO_SPACE) {
        return;
    }
    segment.allocator->free(alloc);
    segment.localUsedBytes -= alignedSize;
    segment.localFreeBytes += alignedSize;

    // 守恒断言：OffsetAllocator 报告的空闲空间应与本地记账一致
    const u64 reported = segment.allocator->storageReport().totalFreeSpace;
    MC_ASSERT_RELEASE_MSG(reported == segment.localFreeBytes,
        "ChunkRenderer mega-buffer segment conservation mismatch: OffsetAllocator free space disagrees with local "
        "bookkeeping");
}

Result<void> ChunkRenderer::_uploadViaStagingPool(
    const void* sourceData, u64 size, VkBuffer dstBuffer, VkDeviceSize dstOffset)
{
    if (sourceData == nullptr || dstBuffer == VK_NULL_HANDLE || size == 0) {
        return Error(ErrorCode::InvalidArgument, "ChunkRenderer::_uploadViaStagingPool: invalid arguments");
    }

    auto* pool = m_context ? m_context->stagingPool() : nullptr;
    if (pool == nullptr) {
        return Error(ErrorCode::NotInitialized, "ChunkRenderer: staging pool not available");
    }

    // 异步上传：stage→memcpy→submitAsyncCopy。submitAsyncCopy 内部分配一次性命令缓冲
    // 录制 vkCmdCopyBuffer 并 submit 带 fence 但不等待，handle+fence+cmd 入池的待回收队列。
    // staging 区间由 stagingPool->pollAsyncCopies() 在 fence signaled 后回收（每帧由
    // TridentEngine::render 推进），调用方不 release。源数据 memcpy 在 submit 前完成
    // （CPU→CPU），submitAsyncCopy 返回后调用方即可释放源 MeshData。
    auto handle = pool->stage(size);
    if (!handle.valid) {
        return Error(ErrorCode::OutOfMemory, "ChunkRenderer: staging pool out of space");
    }
    std::memcpy(handle.mappedPtr, sourceData, static_cast<size_t>(size));
    auto result = pool->submitAsyncCopy(handle, dstBuffer, dstOffset);
    if (!result.success()) {
        // submit 失败（命令缓冲/fence 分配或 submit 出错）：staging 区间尚未入队，
        // 由调用方立即归还避免泄漏。
        pool->release(handle);
    }
    return result;
}

} // namespace mc::client
