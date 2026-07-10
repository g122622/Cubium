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
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <algorithm>
#include <cstring>
#include <glm/geometric.hpp>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::client {

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
        vkFreeMemory(device, indexMemory, nullptr);
        indexMemory = VK_NULL_HANDLE;
    }
    if (vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vertexBuffer = VK_NULL_HANDLE;
    }
    if (vertexMemory != VK_NULL_HANDLE) {
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
    // 清空待上传队列
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        while (!m_pendingUploads.empty()) {
            m_pendingUploads.pop();
        }
    }

    // 清理 Fence 管理器
    m_fenceManager.destroy(m_device, m_commandPool);

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
        std::lock_guard<std::mutex> lock(m_pendingDestroysMutex);
        for (auto& pending : m_pendingDestroys) {
            if (pending.buffer) {
                pending.buffer->destroy(m_device);
            }
        }
        m_pendingDestroys.clear();
    }

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
                std::lock_guard<std::mutex> lock(m_pendingDestroysMutex);
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
            std::lock_guard<std::mutex> lock(m_pendingDestroysMutex);
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
        std::lock_guard<std::mutex> lock(m_pendingDestroysMutex);
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
    for (u32 index : meshData.indices) {
        if (index >= meshData.vertices.size()) {
            return Error(ErrorCode::InvalidData,
                "Chunk mesh index out of range for chunk (" + std::to_string(buffer.chunkId.x) + ", " +
                    std::to_string(buffer.chunkId.z) + ")");
        }
    }

    const u32 oldVertexCount = buffer.vertexCount;
    const u32 oldIndexCount = buffer.indexCount;

    VkDeviceSize vertexSize = static_cast<VkDeviceSize>(meshData.vertices.size() * sizeof(Vertex));
    VkDeviceSize indexSize = static_cast<VkDeviceSize>(meshData.indices.size() * sizeof(u32));

    // 不在原地覆写正在使用中的 GPU 缓冲区。
    // 即使容量足够，也创建新缓冲并延迟销毁旧缓冲，避免与在飞帧并发访问导致 device lost。
    bool needNewVertex = true;
    bool needNewIndex = true;

    // 创建顶点缓冲区
    if (needNewVertex) {
        if (buffer.vertexBuffer != VK_NULL_HANDLE) {
            // 延迟销毁旧缓冲区，避免 GPU 仍在使用时被提前释放导致 device lost
            auto oldBuffer = std::make_unique<ChunkGpuBuffer>();
            oldBuffer->vertexBuffer = buffer.vertexBuffer;
            oldBuffer->vertexMemory = buffer.vertexMemory;
            oldBuffer->chunkId = buffer.chunkId;
            oldBuffer->vertexCount = oldVertexCount;
            oldBuffer->isValid = true;

            {
                std::lock_guard<std::mutex> lock(m_pendingDestroysMutex);
                PendingBufferDestroy pending;
                pending.buffer = std::move(oldBuffer);
                pending.frameIndex = m_destroyFrameCounter;
                m_pendingDestroys.push_back(std::move(pending));
            }

            buffer.vertexBuffer = VK_NULL_HANDLE;
            buffer.vertexMemory = VK_NULL_HANDLE;
        }

        auto result = _createBuffer(vertexSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            buffer.vertexBuffer,
            buffer.vertexMemory);

        if (result.failed()) {
            return Error(
                ErrorCode::InitializationFailed, "Failed to create vertex buffer: " + result.error().message());
        }
    }

    // 创建索引缓冲区
    if (needNewIndex) {
        if (buffer.indexBuffer != VK_NULL_HANDLE) {
            // 延迟销毁旧缓冲区，避免 GPU 仍在使用时被提前释放导致 device lost
            auto oldBuffer = std::make_unique<ChunkGpuBuffer>();
            oldBuffer->indexBuffer = buffer.indexBuffer;
            oldBuffer->indexMemory = buffer.indexMemory;
            oldBuffer->chunkId = buffer.chunkId;
            oldBuffer->indexCount = oldIndexCount;
            oldBuffer->isValid = true;

            {
                std::lock_guard<std::mutex> lock(m_pendingDestroysMutex);
                PendingBufferDestroy pending;
                pending.buffer = std::move(oldBuffer);
                pending.frameIndex = m_destroyFrameCounter;
                m_pendingDestroys.push_back(std::move(pending));
            }

            buffer.indexBuffer = VK_NULL_HANDLE;
            buffer.indexMemory = VK_NULL_HANDLE;
        }

        auto result = _createBuffer(indexSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            buffer.indexBuffer,
            buffer.indexMemory);

        if (result.failed()) {
            return Error(ErrorCode::InitializationFailed, "Failed to create index buffer: " + result.error().message());
        }
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
            return Error(ErrorCode::OperationFailed, "Failed to create staging buffer");
        }
    }

    // 一次映射，分别写入顶点段和索引段，避免覆盖导致网格损坏
    void* mapped = nullptr;
    VkResult mapResult = vkMapMemory(m_device, m_stagingMemory, 0, stagingLayout.totalSize, 0, &mapped);
    if (mapResult != VK_SUCCESS || mapped == nullptr) {
        _endSingleTimeCommands(commandBuffer);
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
        vkCmdBindIndexBuffer(commandBuffer, buffer->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

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
        vkCmdBindIndexBuffer(commandBuffer, buffer->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

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
        vkCmdBindIndexBuffer(commandBuffer, buffer->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

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

// ============================================================================
// 异步 GPU 上传
// ============================================================================

void ChunkRenderer::submitMeshUpload(const ChunkId& chunkId, MeshData&& meshData)
{
    PendingMeshUpload upload;
    upload.chunkId = chunkId;
    upload.meshData = std::move(meshData);
    upload.submitTime = m_uploadTimestamp++;

    std::lock_guard<std::mutex> lock(m_pendingMutex);
    m_pendingUploads.push(std::move(upload));
}

u32 ChunkRenderer::processPendingUploads(u32 maxPerFrame)
{
    // 首先检查已完成的 fence，回收资源
    m_fenceManager.cleanup(m_device, m_commandPool);

    u32 processed = 0;

    while (processed < maxPerFrame) {
        PendingMeshUpload upload;

        {
            std::lock_guard<std::mutex> lock(m_pendingMutex);
            if (m_pendingUploads.empty()) {
                break;
            }
            upload = std::move(m_pendingUploads.front());
            m_pendingUploads.pop();
        }

        // 同步上传
        auto result = updateChunk(upload.chunkId, upload.meshData);
        if (result.success()) {
            ++processed;
        } else {
            spdlog::warn("Failed to upload mesh for chunk ({}, {}): {}",
                upload.chunkId.x,
                upload.chunkId.z,
                result.error().message());
        }
    }

    return processed;
}

size_t ChunkRenderer::pendingUploadCount() const
{
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    return m_pendingUploads.size();
}

void ChunkRenderer::processPendingDestroys(u32 framesToKeep)
{
    std::lock_guard<std::mutex> lock(m_pendingDestroysMutex);

    // 递增帧计数器
    u64 currentCounter = m_destroyFrameCounter++;

    // 销毁超过保留帧数的缓冲区
    auto it = m_pendingDestroys.begin();
    while (it != m_pendingDestroys.end()) {
        u64 frameDiff = currentCounter >= it->frameIndex ? currentCounter - it->frameIndex
                                                         : (UINT64_MAX - it->frameIndex) + currentCounter + 1;

        if (frameDiff >= framesToKeep) {
            if (it->buffer) {
                it->buffer->destroy(m_device);
            }
            it = m_pendingDestroys.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// Fence 管理器
// ============================================================================

void FenceManager::cleanup(VkDevice device, VkCommandPool commandPool)
{
    for (size_t i = 0; i < inUse.size(); ++i) {
        if (inUse[i] && fences[i] != VK_NULL_HANDLE) {
            VkResult result = vkGetFenceStatus(device, fences[i]);
            if (result == VK_SUCCESS) {
                vkDestroyFence(device, fences[i], nullptr);
                fences[i] = VK_NULL_HANDLE;

                if (commandBuffers[i] != VK_NULL_HANDLE) {
                    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffers[i]);
                    commandBuffers[i] = VK_NULL_HANDLE;
                }

                inUse[i] = false;
            }
        }
    }
}

void FenceManager::destroy(VkDevice device, VkCommandPool commandPool)
{
    for (size_t i = 0; i < fences.size(); ++i) {
        if (fences[i] != VK_NULL_HANDLE) {
            vkWaitForFences(device, 1, &fences[i], VK_TRUE, UINT64_MAX);
            vkDestroyFence(device, fences[i], nullptr);
        }
        if (commandBuffers[i] != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(device, commandPool, 1, &commandBuffers[i]);
        }
    }
    fences.clear();
    commandBuffers.clear();
    inUse.clear();
    nextIndex = 0;
}

} // namespace mc::client
