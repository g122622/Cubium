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
// ChunkGpuBuffer 的 vertexMemory/indexMemory 是 GPU 句柄成员，其生命周期跨越
// 「_createBuffer 写入 → 替换路径字段级转移到 oldBuffer → destroy 释放」，地址
// （成员存储地址）随宿主对象走。这里用手动 MC_TRACE_MEM_ALLOC/FREE 宏追踪这些
// 稳定地址（vkAllocateMemory/vkFreeMemory 严格一对一），用 helper 封装保证配对。
//
// Tracy 按 name 指针区分内存池（见 tracy 手册 3.1.2 节），故 name 必须是同一字面量
// 的稳定指针。下面用文件作用域 const char* const 变量持有，保证 alloc/free 传入同一指针。

namespace {

/// 顶点缓冲区 GPU 内存池名
const char* const kGpuPoolChunkVtx = "ChunkVtx";
/// 索引缓冲区 GPU 内存池名
const char* const kGpuPoolChunkIdx = "ChunkIdx";

/// 登记一次 GPU 内存分配。memPtr 指向 ChunkGpuBuffer 中存储 VkDeviceMemory 句柄的成员地址。
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
// ChunkGpuBuffer 实现
// ============================================================================

void ChunkGpuBuffer::destroy(VkDevice device)
{
    if (indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, indexBuffer, nullptr);
        indexBuffer = VK_NULL_HANDLE;
    }
    if (indexMemory != VK_NULL_HANDLE) {
        // GPU 内存追踪：与 _createChunkBuffer 中的 alloc 严格一对一（同一 &indexMemory 地址）。
        // 守卫保证仅对已 alloc 的句柄 free，避免未分配（NULL_HANDLE）时误 free。
        trackGpuFree(&indexMemory, kGpuPoolChunkIdx);
        vkFreeMemory(device, indexMemory, nullptr);
        indexMemory = VK_NULL_HANDLE;
    }
    if (vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vertexBuffer = VK_NULL_HANDLE;
    }
    if (vertexMemory != VK_NULL_HANDLE) {
        // GPU 内存追踪：与 _createChunkBuffer 中的 alloc 严格一对一（同一 &vertexMemory 地址）。
        trackGpuFree(&vertexMemory, kGpuPoolChunkVtx);
        vkFreeMemory(device, vertexMemory, nullptr);
        vertexMemory = VK_NULL_HANDLE;
    }
    indexCount = 0;
    vertexCount = 0;
    isValid = false;
}

// ============================================================================
// ChunkTextureAtlas 实现
// ============================================================================

void ChunkTextureAtlas::destroy(VkDevice device)
{
    if (sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler, nullptr);
        sampler = VK_NULL_HANDLE;
    }
    if (imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, imageView, nullptr);
        imageView = VK_NULL_HANDLE;
    }
    if (image != VK_NULL_HANDLE) {
        vkDestroyImage(device, image, nullptr);
        image = VK_NULL_HANDLE;
    }
    if (memory != VK_NULL_HANDLE) {
        // GPU 内存追踪：与 _createTextureAtlas 中的 alloc 严格一对一（同一 &memory 地址）。
        // 守卫保证仅对已 alloc 的句柄 free，避免首次创建（memory 为 null）时误 free。
        MC_TRACE_MEM_FREE("ChunkAtlas", &memory);
        vkFreeMemory(device, memory, nullptr);
        memory = VK_NULL_HANDLE;
    }
    isValid = false;
}

TextureRegion ChunkTextureAtlas::getRegion(u32 tileX, u32 tileY) const
{
    TextureRegion region;
    region.u0 = static_cast<f64>(tileX * tileSize) / static_cast<f64>(width);
    region.v0 = static_cast<f64>(tileY * tileSize) / static_cast<f64>(height);
    region.u1 = region.u0 + tileU;
    region.v1 = region.v0 + tileV;
    return region;
}

TextureRegion ChunkTextureAtlas::getRegion(u32 tileIndex) const
{
    u32 tileX = tileIndex % tilesPerRow;
    u32 tileY = tileIndex / tilesPerRow;
    return getRegion(tileX, tileY);
}

// ============================================================================
// ChunkRenderer 实现
// ============================================================================

ChunkRenderer::ChunkRenderer() = default;

ChunkRenderer::~ChunkRenderer()
{
    destroy();
}

Result<void> ChunkRenderer::initialize(
    VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, u32 maxChunks)
{
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
    // 销毁所有区块缓冲区
    for (auto& pair : m_chunkBuffers) {
        if (pair.second) {
            pair.second->destroy(m_device);
        }
    }
    m_chunkBuffers.clear();
    m_totalVertices = 0;
    m_totalIndices = 0;

    // 清理延迟销毁队列
    {
        std::lock_guard<std::recursive_mutex> lock(m_pendingDestroysMutex);
        for (auto& pending : m_pendingDestroys) {
            if (pending.buffer) {
                pending.buffer->destroy(m_device);
            }
        }
        m_pendingDestroys.clear();
    }

    // 销毁 free-list 中所有缓存 buffer（真销毁，配对 Tracy free）
    _destroyFreeList();

    // 销毁暂存缓冲区
    if (m_stagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_stagingBuffer, nullptr);
        m_stagingBuffer = VK_NULL_HANDLE;
    }
    if (m_stagingMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_stagingMemory, nullptr);
        m_stagingMemory = VK_NULL_HANDLE;
    }

    // 销毁纹理图集
    m_textureAtlas.destroy(m_device);

    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_graphicsQueue = VK_NULL_HANDLE;
}

Result<void> ChunkRenderer::_createBuffer(VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& memory)
{
    return renderer::VulkanUtils::createBuffer(m_device, m_physicalDevice, size, usage, properties, buffer, memory);
}

Result<u32> ChunkRenderer::_findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    return renderer::VulkanUtils::findMemoryType(m_physicalDevice, typeFilter, properties);
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

            {
                std::lock_guard<std::recursive_mutex> lock(m_pendingDestroysMutex);
                PendingBufferDestroy pending;
                pending.buffer = std::move(it->second);
                pending.frameIndex = m_destroyFrameCounter;
                m_pendingDestroys.push_back(std::move(pending));
            }

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

        {
            std::lock_guard<std::recursive_mutex> lock(m_pendingDestroysMutex);
            PendingBufferDestroy pending;
            pending.buffer = std::move(it->second);
            pending.frameIndex = m_destroyFrameCounter;
            m_pendingDestroys.push_back(std::move(pending));
        }

        m_chunkBuffers.erase(it);
    }
}

void ChunkRenderer::clearChunks()
{
    // 将所有缓冲区移入延迟销毁队列
    {
        std::lock_guard<std::recursive_mutex> lock(m_pendingDestroysMutex);
        for (auto& pair : m_chunkBuffers) {
            if (pair.second && pair.second->isValid) {
                PendingBufferDestroy pending;
                pending.buffer = std::move(pair.second);
                pending.frameIndex = m_destroyFrameCounter;
                m_pendingDestroys.push_back(std::move(pending));
            }
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

    VkDeviceSize vertexSize = static_cast<VkDeviceSize>(meshData.vertices.size() * sizeof(Vertex));
    VkDeviceSize indexSize = static_cast<VkDeviceSize>(meshData.indices.size() * sizeof(u16));

    // 旧 buffer（vertex+index 一起）摘入延迟销毁队列：等在飞帧 draw 完成后（processPendingDestroys
    // 到期）转入 free-list 供复用，而非直接销毁。这样既避免原地覆写与在飞帧并发访问的 device lost
    // 风险，又能复用容量、消除峰期新旧 buffer 共存翻倍。
    if (buffer.vertexBuffer != VK_NULL_HANDLE || buffer.indexBuffer != VK_NULL_HANDLE) {
        // GPU 内存追踪：追踪权随句柄走。先 free 旧持有地址（&buffer.vertexMemory/&indexMemory），
        // 句柄值转移给 oldBuffer 后在新地址重新登记 alloc，使后续 processPendingDestroys →
        // _releaseToFreeList 的 free/alloc 转移能正确配对。size 用 capacity（反映 buffer 真实占用）。
        if (buffer.vertexMemory != VK_NULL_HANDLE) {
            trackGpuFree(&buffer.vertexMemory, kGpuPoolChunkVtx);
        }
        if (buffer.indexMemory != VK_NULL_HANDLE) {
            trackGpuFree(&buffer.indexMemory, kGpuPoolChunkIdx);
        }

        auto oldBuffer = std::make_unique<ChunkGpuBuffer>();
        oldBuffer->vertexBuffer = buffer.vertexBuffer;
        oldBuffer->vertexMemory = buffer.vertexMemory;
        oldBuffer->indexBuffer = buffer.indexBuffer;
        oldBuffer->indexMemory = buffer.indexMemory;
        oldBuffer->vertexCapacity = buffer.vertexCapacity;
        oldBuffer->indexCapacity = buffer.indexCapacity;
        oldBuffer->chunkId = buffer.chunkId;
        oldBuffer->vertexCount = oldVertexCount;
        oldBuffer->indexCount = oldIndexCount;
        oldBuffer->isValid = true;

        if (oldBuffer->vertexMemory != VK_NULL_HANDLE) {
            trackGpuAlloc(&oldBuffer->vertexMemory, oldBuffer->vertexCapacity, kGpuPoolChunkVtx);
        }
        if (oldBuffer->indexMemory != VK_NULL_HANDLE) {
            trackGpuAlloc(&oldBuffer->indexMemory, oldBuffer->indexCapacity, kGpuPoolChunkIdx);
        }

        {
            std::lock_guard<std::recursive_mutex> lock(m_pendingDestroysMutex);
            PendingBufferDestroy pending;
            pending.buffer = std::move(oldBuffer);
            pending.frameIndex = m_destroyFrameCounter;
            m_pendingDestroys.push_back(std::move(pending));
        }

        buffer.vertexBuffer = VK_NULL_HANDLE;
        buffer.vertexMemory = VK_NULL_HANDLE;
        buffer.indexBuffer = VK_NULL_HANDLE;
        buffer.indexMemory = VK_NULL_HANDLE;
        buffer.vertexCapacity = 0;
        buffer.indexCapacity = 0;
    }

    // 优先从 free-list 复用整 buffer（vertex+index 一起），避免重复 vkAllocateMemory。
    // 未命中时退回 _createBuffer 新建。两种路径都需后续 staging 重传 data。
    const bool reused = _acquireFromFreeList(vertexSize, indexSize, buffer);

    if (!reused) {
        auto vtxResult = _createBuffer(vertexSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            buffer.vertexBuffer,
            buffer.vertexMemory);

        if (vtxResult.failed()) {
            return Error(
                ErrorCode::InitializationFailed, "Failed to create vertex buffer: " + vtxResult.error().message());
        }

        // GPU 内存追踪：_createBuffer 已 vkAllocateMemory 写入 buffer.vertexMemory，登记 alloc。
        trackGpuAlloc(&buffer.vertexMemory, vertexSize, kGpuPoolChunkVtx);
        buffer.vertexCapacity = vertexSize;

        auto idxResult = _createBuffer(indexSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            buffer.indexBuffer,
            buffer.indexMemory);

        if (idxResult.failed()) {
            // 失败清理：vertex 已 alloc 成功（&buffer.vertexMemory 已登记），此处 destroy 释放
            // vertex 句柄并对应 free 配对；index 句柄因创建失败仍为 NULL_HANDLE，destroy 守卫跳过。
            // 避免 GPU 句柄与追踪事件泄漏（调用方不会再 destroy 本 buffer），杜绝堆地址复用导致的
            // Tracy MemAllocTwice 硬失败。
            buffer.destroy(m_device);
            return Error(
                ErrorCode::InitializationFailed, "Failed to create index buffer: " + idxResult.error().message());
        }

        // GPU 内存追踪：_createBuffer 已 vkAllocateMemory 写入 buffer.indexMemory，登记 alloc。
        trackGpuAlloc(&buffer.indexMemory, indexSize, kGpuPoolChunkIdx);
        buffer.indexCapacity = indexSize;
    }

    // 上传数据
    auto cmdResult = _beginSingleTimeCommands();
    if (!cmdResult.success()) {
        buffer.destroy(m_device);
        return cmdResult.error();
    }
    VkCommandBuffer commandBuffer = cmdResult.value();

    // 使用分段上传布局，确保顶点和索引在暂存缓冲区内不重叠
    const StagingCopyLayout stagingLayout = buildStagingCopyLayout(vertexSize, indexSize);

    // 确保暂存缓冲区足够大
    if (stagingLayout.totalSize > m_stagingBufferSize || m_stagingBuffer == VK_NULL_HANDLE) {
        // 销毁旧的暂存缓冲区
        if (m_stagingBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, m_stagingBuffer, nullptr);
            vkFreeMemory(m_device, m_stagingMemory, nullptr);
        }

        m_stagingBufferSize = std::max(stagingLayout.totalSize, static_cast<VkDeviceSize>(16 * 1024 * 1024));
        auto result = _createBuffer(m_stagingBufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_stagingBuffer,
            m_stagingMemory);

        if (result.failed()) {
            _endSingleTimeCommands(commandBuffer);
            // 失败清理：vertex/index 句柄均已 alloc 登记，destroy 释放并配对 free，避免泄漏。
            buffer.destroy(m_device);
            return Error(ErrorCode::OperationFailed, "Failed to create staging buffer");
        }
    }

    // 一次映射，分别写入顶点段和索引段，避免覆盖导致网格损坏
    void* mapped = nullptr;
    VkResult mapResult = vkMapMemory(m_device, m_stagingMemory, 0, stagingLayout.totalSize, 0, &mapped);
    if (mapResult != VK_SUCCESS || mapped == nullptr) {
        _endSingleTimeCommands(commandBuffer);
        // 失败清理：同上，destroy 释放已登记的 vertex/index alloc。
        buffer.destroy(m_device);
        return Error(ErrorCode::OperationFailed, "Failed to map staging buffer memory");
    }

    u8* stagingBytes = static_cast<u8*>(mapped);
    std::memcpy(stagingBytes + stagingLayout.vertexOffset, meshData.vertices.data(), static_cast<size_t>(vertexSize));
    std::memcpy(stagingBytes + stagingLayout.indexOffset, meshData.indices.data(), static_cast<size_t>(indexSize));
    vkUnmapMemory(m_device, m_stagingMemory);

    // 复制顶点数据
    VkBufferCopy vertexCopy{};
    vertexCopy.srcOffset = stagingLayout.vertexOffset;
    vertexCopy.dstOffset = 0;
    vertexCopy.size = vertexSize;
    vkCmdCopyBuffer(commandBuffer, m_stagingBuffer, buffer.vertexBuffer, 1, &vertexCopy);

    // 复制索引数据
    VkBufferCopy indexCopy{};
    indexCopy.srcOffset = stagingLayout.indexOffset;
    indexCopy.dstOffset = 0;
    indexCopy.size = indexSize;
    vkCmdCopyBuffer(commandBuffer, m_stagingBuffer, buffer.indexBuffer, 1, &indexCopy);

    _endSingleTimeCommands(commandBuffer);

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

Result<void> ChunkRenderer::loadTextureAtlas(const u8* pixelData, u32 width, u32 height, u32 tileSize)
{
    if (pixelData == nullptr) {
        return Error(ErrorCode::NullPointer, "Texture atlas pixel data is null");
    }

    if (width == 0 || height == 0 || tileSize == 0) {
        return Error(ErrorCode::InvalidArgument, "Invalid texture atlas dimensions or tile size");
    }

    if (width < tileSize || height < tileSize) {
        return Error(ErrorCode::InvalidArgument, "Texture atlas is smaller than tile size");
    }

    // 创建纹理图集
    auto result = _createTextureAtlas(width, height);
    if (result.failed()) {
        return result;
    }

    m_textureAtlas.tileSize = tileSize;
    m_textureAtlas.tilesPerRow = width / tileSize;
    m_textureAtlas.tileU = static_cast<f64>(tileSize) / static_cast<f64>(width);
    m_textureAtlas.tileV = static_cast<f64>(tileSize) / static_cast<f64>(height);

    // 上传纹理数据
    return _uploadTextureData(pixelData, width, height);
}

Result<void> ChunkRenderer::_createTextureAtlas(u32 width, u32 height)
{
    // 销毁旧纹理
    m_textureAtlas.destroy(m_device);

    // 创建图像
    auto imageResult = renderer::VulkanUtils::createImage(m_device,
        m_physicalDevice,
        width,
        height,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_textureAtlas.image,
        m_textureAtlas.memory);

    if (imageResult.failed()) {
        return imageResult.error();
    }

    // GPU 内存追踪：ChunkRenderer 不可移动，&m_textureAtlas.memory 地址稳定，无 move 风险。
    // createImage 内部已 vkAllocateMemory 写入 m_textureAtlas.memory，此处查 requirements
    // 取真实 size 后登记 alloc。与 ChunkTextureAtlas::destroy 中的 free 严格一对一。
    {
        VkMemoryRequirements memRequirements{};
        vkGetImageMemoryRequirements(m_device, m_textureAtlas.image, &memRequirements);
        MC_TRACE_MEM_ALLOC("ChunkAtlas", &m_textureAtlas.memory, memRequirements.size);
    }

    // 创建图像视图
    auto viewResult = renderer::VulkanUtils::createImageView(
        m_device, m_textureAtlas.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_textureAtlas.imageView);

    if (viewResult.failed()) {
        m_textureAtlas.destroy(m_device);
        return viewResult.error();
    }

    // 创建采样器
    auto samplerResult = renderer::VulkanUtils::createSampler(m_device,
        VK_FILTER_NEAREST,
        VK_FILTER_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        m_textureAtlas.sampler);

    if (samplerResult.failed()) {
        m_textureAtlas.destroy(m_device);
        return samplerResult.error();
    }

    m_textureAtlas.width = width;
    m_textureAtlas.height = height;
    m_textureAtlas.isValid = true;

    return {};
}

Result<void> ChunkRenderer::_uploadTextureData(const u8* pixelData, u32 width, u32 height)
{
    if (pixelData == nullptr) {
        return Error(ErrorCode::NullPointer, "Texture atlas pixel data is null");
    }

    const u64 imageSize64 = static_cast<u64>(width) * static_cast<u64>(height) * 4ULL;
    if (imageSize64 == 0) {
        return Error(ErrorCode::InvalidArgument, "Texture atlas image size is zero");
    }

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(imageSize64);

    // 创建暂存缓冲区
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    auto result = _createBuffer(imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory);

    if (result.failed()) {
        return result;
    }

    // 映射并复制数据
    void* mapped = nullptr;
    VkResult mapResult = vkMapMemory(m_device, stagingMemory, 0, imageSize, 0, &mapped);
    if (mapResult != VK_SUCCESS || mapped == nullptr) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
        return Error(ErrorCode::OperationFailed, "Failed to map staging buffer memory");
    }
    std::memcpy(mapped, pixelData, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_device, stagingMemory);

    // 转换图像布局并复制
    auto cmdResult = _beginSingleTimeCommands();
    if (cmdResult.failed()) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
        return cmdResult.error();
    }
    VkCommandBuffer cmd = cmdResult.value();

    // 转换到传输目标布局
    renderer::VulkanUtils::transitionImageLayout(cmd,
        m_textureAtlas.image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    // 复制缓冲区到图像
    renderer::VulkanUtils::copyBufferToImage(cmd, stagingBuffer, m_textureAtlas.image, width, height);

    // 转换到着色器只读布局
    renderer::VulkanUtils::transitionImageLayout(cmd,
        m_textureAtlas.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    _endSingleTimeCommands(cmd);

    // 清理暂存缓冲区
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingMemory, nullptr);

    return {};
}

Result<void> ChunkRenderer::uploadTextureRegion(
    const void* pixelData, u64 size, u32 offsetX, u32 offsetY, u32 width, u32 height, u32 rowLength)
{
    if (pixelData == nullptr) {
        return Error(ErrorCode::NullPointer, "Texture region pixel data is null");
    }

    if (m_textureAtlas.image == VK_NULL_HANDLE) {
        return Error(ErrorCode::InvalidState, "Texture atlas not initialized");
    }

    if (width == 0 || height == 0) {
        return Error(ErrorCode::InvalidArgument, "Texture region dimensions must be non-zero");
    }

    // 创建暂存缓冲区
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    auto result = _createBuffer(static_cast<VkDeviceSize>(size),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory);

    if (result.failed()) {
        return result;
    }

    // 映射并复制数据
    void* mapped = nullptr;
    VkResult mapResult = vkMapMemory(m_device, stagingMemory, 0, static_cast<VkDeviceSize>(size), 0, &mapped);
    if (mapResult != VK_SUCCESS || mapped == nullptr) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
        return Error(ErrorCode::OperationFailed, "Failed to map staging buffer memory for region upload");
    }
    std::memcpy(mapped, pixelData, static_cast<size_t>(size));
    vkUnmapMemory(m_device, stagingMemory);

    // 开始命令缓冲区
    auto cmdResult = _beginSingleTimeCommands();
    if (cmdResult.failed()) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
        return cmdResult.error();
    }
    VkCommandBuffer cmd = cmdResult.value();

    // 转换到传输目标布局
    renderer::VulkanUtils::transitionImageLayout(cmd,
        m_textureAtlas.image,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    // 复制缓冲区到图像子区域
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = rowLength > 0 ? rowLength : width;
    region.bufferImageHeight = height;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {static_cast<int32_t>(offsetX), static_cast<int32_t>(offsetY), 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, m_textureAtlas.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // 转换回着色器只读布局
    renderer::VulkanUtils::transitionImageLayout(cmd,
        m_textureAtlas.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    _endSingleTimeCommands(cmd);

    // 清理暂存缓冲区
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingMemory, nullptr);

    return {};
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
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, buffer->indexBuffer, 0, VK_INDEX_TYPE_UINT16);

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
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, buffer->indexBuffer, 0, VK_INDEX_TYPE_UINT16);

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
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, buffer->indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdDrawIndexed(commandBuffer, buffer->indexCount, 1, 0, 0, 0);
    }
}

Result<VkCommandBuffer> ChunkRenderer::_beginSingleTimeCommands()
{
    VkCommandBuffer cmd = renderer::VulkanUtils::beginSingleTimeCommands(m_device, m_commandPool);
    if (cmd == VK_NULL_HANDLE) {
        return Error(ErrorCode::OperationFailed, "Failed to allocate command buffer");
    }
    return cmd;
}

void ChunkRenderer::_endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    // 使用 fence 版本，避免阻塞整个 GPU 队列
    renderer::VulkanUtils::endSingleTimeCommands(m_device, m_commandPool, m_graphicsQueue, commandBuffer);
}

void ChunkRenderer::processPendingDestroys(u32 framesToKeep)
{
    std::lock_guard<std::recursive_mutex> lock(m_pendingDestroysMutex);

    // 递增帧计数器
    u64 currentCounter = m_destroyFrameCounter++;

    // 冷却完成的缓冲区转入 free-list 供复用，而非直接销毁。
    // 此时已过 framesToKeep 帧（远超 MAX_FRAMES_IN_FLIGHT），在飞帧 draw 早已完成，
    // buffer 既可销毁也可安全复用。
    auto it = m_pendingDestroys.begin();
    while (it != m_pendingDestroys.end()) {
        u64 frameDiff = currentCounter >= it->frameIndex ? currentCounter - it->frameIndex
                                                         : (UINT64_MAX - it->frameIndex) + currentCounter + 1;

        if (frameDiff >= framesToKeep) {
            if (it->buffer) {
                ChunkGpuBuffer& b = *it->buffer;
                // GPU 内存追踪：追踪权从 oldBuffer 地址转移到 free-list entry 地址。
                // 先 free oldBuffer 持有地址，_releaseToFreeList 内部在 entry 新地址上重新 alloc。
                if (b.vertexMemory != VK_NULL_HANDLE) {
                    trackGpuFree(&b.vertexMemory, kGpuPoolChunkVtx);
                }
                if (b.indexMemory != VK_NULL_HANDLE) {
                    trackGpuFree(&b.indexMemory, kGpuPoolChunkIdx);
                }
                _releaseToFreeList(
                    b.vertexBuffer, b.vertexMemory, b.indexBuffer, b.indexMemory, b.vertexCapacity, b.indexCapacity);
            }
            it = m_pendingDestroys.erase(it);
        } else {
            ++it;
        }
    }
}

bool ChunkRenderer::_acquireFromFreeList(VkDeviceSize vertexSize, VkDeviceSize indexSize, ChunkGpuBuffer& out)
{
    std::lock_guard<std::recursive_mutex> lock(m_pendingDestroysMutex);

    // best-fit：lower_bound 找首个 vertexCapacity >= vertexSize 的候选。
    auto it = m_freeList.lower_bound(vertexSize);
    while (it != m_freeList.end()) {
        FreeListEntry& entry = it->second;
        // vertex 满足后校验 index（二者按比例增长，通常同时满足；不满足则继续找更大的候选）。
        if (entry.indexCapacity >= indexSize) {
            // 命中：句柄转移到 out，Tracy 追踪权从 entry 地址转移到 out 地址。
            // size 沿用 entry 的 capacity（反映 buffer 真实占用，与新 data size 可能不同）。
            trackGpuFree(&entry.vertexMemory, kGpuPoolChunkVtx);
            trackGpuFree(&entry.indexMemory, kGpuPoolChunkIdx);

            out.vertexBuffer = entry.vertexBuffer;
            out.vertexMemory = entry.vertexMemory;
            out.indexBuffer = entry.indexBuffer;
            out.indexMemory = entry.indexMemory;
            out.vertexCapacity = entry.vertexCapacity;
            out.indexCapacity = entry.indexCapacity;

            trackGpuAlloc(&out.vertexMemory, entry.vertexCapacity, kGpuPoolChunkVtx);
            trackGpuAlloc(&out.indexMemory, entry.indexCapacity, kGpuPoolChunkIdx);

            m_freeListTotalBytes -= (entry.vertexCapacity + entry.indexCapacity);
            m_freeList.erase(it);
            return true;
        }
        ++it;
    }
    return false;
}

void ChunkRenderer::_releaseToFreeList(VkBuffer vertexBuffer,
    VkDeviceMemory vertexMemory,
    VkBuffer indexBuffer,
    VkDeviceMemory indexMemory,
    VkDeviceSize vertexCapacity,
    VkDeviceSize indexCapacity)
{
    std::lock_guard<std::recursive_mutex> lock(m_pendingDestroysMutex);

    const VkDeviceSize incomingBytes = vertexCapacity + indexCapacity;

    // 超 limit 前先按 FIFO 淘汰最旧条目（真销毁），为新条目腾位。
    // 淘汰需配对 Tracy free（entry 当初从 pending 转入时在新地址 alloc 过）。
    while (!m_freeList.empty() &&
        (m_freeList.size() >= kFreeListMaxCount || m_freeListTotalBytes + incomingBytes > kFreeListMaxBytes)) {
        auto oldest = m_freeList.begin();
        for (auto scan = m_freeList.begin(); scan != m_freeList.end(); ++scan) {
            if (scan->second.enlistSeq < oldest->second.enlistSeq) {
                oldest = scan;
            }
        }
        m_freeListTotalBytes -= (oldest->second.vertexCapacity + oldest->second.indexCapacity);
        _destroyFreeListEntry(oldest->second);
        m_freeList.erase(oldest);
    }

    FreeListEntry entry;
    entry.vertexBuffer = vertexBuffer;
    entry.vertexMemory = vertexMemory;
    entry.indexBuffer = indexBuffer;
    entry.indexMemory = indexMemory;
    entry.vertexCapacity = vertexCapacity;
    entry.indexCapacity = indexCapacity;
    entry.enlistSeq = m_freeListEnlistCounter++;
    m_freeListTotalBytes += incomingBytes;

    // multimap 节点地址稳定（不 move），在入池后的稳定地址上重新登记 Tracy alloc，
    // 与调用方（processPendingDestroys）先 free 旧地址配对。multimap::emplace 总是成功，返回 iterator。
    auto nodeIt = m_freeList.emplace(entry.vertexCapacity, std::move(entry));
    FreeListEntry& stored = nodeIt->second;
    if (stored.vertexMemory != VK_NULL_HANDLE) {
        trackGpuAlloc(&stored.vertexMemory, stored.vertexCapacity, kGpuPoolChunkVtx);
    }
    if (stored.indexMemory != VK_NULL_HANDLE) {
        trackGpuAlloc(&stored.indexMemory, stored.indexCapacity, kGpuPoolChunkIdx);
    }
}

void ChunkRenderer::_destroyFreeListEntry(FreeListEntry& entry)
{
    // 真销毁：vkDestroy + vkFree + Tracy free 配对（entry 地址上的 alloc 在入池时已登记）。
    if (entry.vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, entry.vertexBuffer, nullptr);
        entry.vertexBuffer = VK_NULL_HANDLE;
    }
    if (entry.vertexMemory != VK_NULL_HANDLE) {
        trackGpuFree(&entry.vertexMemory, kGpuPoolChunkVtx);
        vkFreeMemory(m_device, entry.vertexMemory, nullptr);
        entry.vertexMemory = VK_NULL_HANDLE;
    }
    if (entry.indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, entry.indexBuffer, nullptr);
        entry.indexBuffer = VK_NULL_HANDLE;
    }
    if (entry.indexMemory != VK_NULL_HANDLE) {
        trackGpuFree(&entry.indexMemory, kGpuPoolChunkIdx);
        vkFreeMemory(m_device, entry.indexMemory, nullptr);
        entry.indexMemory = VK_NULL_HANDLE;
    }
}

void ChunkRenderer::_destroyFreeList()
{
    std::lock_guard<std::recursive_mutex> lock(m_pendingDestroysMutex);
    for (auto& pair : m_freeList) {
        _destroyFreeListEntry(pair.second);
    }
    m_freeList.clear();
    m_freeListTotalBytes = 0;
}

} // namespace mc::client
