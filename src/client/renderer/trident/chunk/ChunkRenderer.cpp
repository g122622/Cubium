#include "ChunkRenderer.hpp"
#include "../util/VulkanUtils.hpp"
#include <spdlog/spdlog.h>
#include <cstring>
#include <algorithm>
#include <glm/glm.hpp>

namespace mc::client {

// ============================================================================
// ChunkGpuBuffer 实现
// ============================================================================

void ChunkGpuBuffer::destroy(VkDevice device) {
    // 销毁实心网格缓冲区
    if (solidIndexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, solidIndexBuffer, nullptr);
        solidIndexBuffer = VK_NULL_HANDLE;
    }
    if (solidIndexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, solidIndexMemory, nullptr);
        solidIndexMemory = VK_NULL_HANDLE;
    }
    if (solidVertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, solidVertexBuffer, nullptr);
        solidVertexBuffer = VK_NULL_HANDLE;
    }
    if (solidVertexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, solidVertexMemory, nullptr);
        solidVertexMemory = VK_NULL_HANDLE;
    }
    solidIndexCount = 0;
    solidVertexCount = 0;

    // 销毁透明网格缓冲区
    if (transparentIndexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, transparentIndexBuffer, nullptr);
        transparentIndexBuffer = VK_NULL_HANDLE;
    }
    if (transparentIndexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, transparentIndexMemory, nullptr);
        transparentIndexMemory = VK_NULL_HANDLE;
    }
    if (transparentVertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, transparentVertexBuffer, nullptr);
        transparentVertexBuffer = VK_NULL_HANDLE;
    }
    if (transparentVertexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, transparentVertexMemory, nullptr);
        transparentVertexMemory = VK_NULL_HANDLE;
    }
    transparentIndexCount = 0;
    transparentVertexCount = 0;

    hasTransparentMesh = false;
    isValid = false;
}

// ============================================================================
// ChunkTextureAtlas 实现
// ============================================================================

void ChunkTextureAtlas::destroy(VkDevice device) {
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

TextureRegion ChunkTextureAtlas::getRegion(u32 tileX, u32 tileY) const {
    TextureRegion region;
    region.u0 = static_cast<f32>(tileX * tileSize) / static_cast<f32>(width);
    region.v0 = static_cast<f32>(tileY * tileSize) / static_cast<f32>(height);
    region.u1 = region.u0 + tileU;
    region.v1 = region.v0 + tileV;
    return region;
}

TextureRegion ChunkTextureAtlas::getRegion(u32 tileIndex) const {
    u32 tileX = tileIndex % tilesPerRow;
    u32 tileY = tileIndex / tilesPerRow;
    return getRegion(tileX, tileY);
}

// ============================================================================
// ChunkRenderer 实现
// ============================================================================

ChunkRenderer::ChunkRenderer() = default;

ChunkRenderer::~ChunkRenderer() {
    destroy();
}

Result<void> ChunkRenderer::initialize(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    u32 maxChunks)
{
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_maxChunks = maxChunks;

    spdlog::info("ChunkRenderer initialized (max chunks: {})", maxChunks);
    return {};
}

void ChunkRenderer::destroy() {
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
    m_totalSolidVertices = 0;
    m_totalSolidIndices = 0;
    m_totalTransparentVertices = 0;
    m_totalTransparentIndices = 0;

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

Result<void> ChunkRenderer::createBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& memory)
{
    return renderer::VulkanUtils::createBuffer(m_device, m_physicalDevice, size, usage, properties, buffer, memory);
}

Result<u32> ChunkRenderer::findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties) {
    return renderer::VulkanUtils::findMemoryType(m_physicalDevice, typeFilter, properties);
}

Result<void> ChunkRenderer::updateChunk(
    const ChunkId& chunkId,
    const MeshData& meshData)
{
    // 向后兼容接口：仅更新实心网格
    return updateChunk(chunkId, meshData, MeshData{});
}

Result<void> ChunkRenderer::updateChunk(
    const ChunkId& chunkId,
    const MeshData& solidMesh,
    const MeshData& transparentMesh)
{
    // 如果两个网格都为空，移除区块
    if (solidMesh.empty() && transparentMesh.empty()) {
        removeChunk(chunkId);
        return {};
    }

    // 查找或创建缓冲区
    u64 id = chunkId.toId();
    auto it = m_chunkBuffers.find(id);

    if (it == m_chunkBuffers.end()) {
        if (m_chunkBuffers.size() >= m_maxChunks) {
            return Error(ErrorCode::CapacityExceeded, "Maximum chunk count reached");
        }

        auto buffer = std::make_unique<ChunkGpuBuffer>();
        buffer->chunkId = chunkId;
        // 计算区块中心位置（用于距离排序）
        buffer->centerPosition = glm::vec3(
            static_cast<float>(chunkId.x) * 16.0f + 8.0f,
            64.0f,  // 使用中间高度
            static_cast<float>(chunkId.z) * 16.0f + 8.0f
        );

        // 创建实心网格缓冲区
        if (!solidMesh.empty()) {
            auto result = createMeshBuffers(
                buffer->solidVertexBuffer,
                buffer->solidVertexMemory,
                buffer->solidIndexBuffer,
                buffer->solidIndexMemory,
                solidMesh,
                buffer->solidVertexCount,
                buffer->solidIndexCount);
            if (!result.success()) {
                return result;
            }
            m_totalSolidVertices += buffer->solidVertexCount;
            m_totalSolidIndices += buffer->solidIndexCount;
        }

        // 创建透明网格缓冲区
        if (!transparentMesh.empty()) {
            auto result = createMeshBuffers(
                buffer->transparentVertexBuffer,
                buffer->transparentVertexMemory,
                buffer->transparentIndexBuffer,
                buffer->transparentIndexMemory,
                transparentMesh,
                buffer->transparentVertexCount,
                buffer->transparentIndexCount);
            if (!result.success()) {
                return result;
            }
            buffer->hasTransparentMesh = true;
            m_totalTransparentVertices += buffer->transparentVertexCount;
            m_totalTransparentIndices += buffer->transparentIndexCount;
        }

        buffer->isValid = true;
        m_chunkBuffers[id] = std::move(buffer);
    } else {
        // 更新现有缓冲区
        auto& buffer = *it->second;

        // 更新实心网格
        u32 oldSolidVertices = buffer.solidVertexCount;
        u32 oldSolidIndices = buffer.solidIndexCount;

        if (!solidMesh.empty()) {
            auto result = createMeshBuffers(
                buffer.solidVertexBuffer,
                buffer.solidVertexMemory,
                buffer.solidIndexBuffer,
                buffer.solidIndexMemory,
                solidMesh,
                buffer.solidVertexCount,
                buffer.solidIndexCount);
            if (!result.success()) {
                return result;
            }
        } else if (buffer.solidVertexBuffer != VK_NULL_HANDLE) {
            // 实心网格已清空，销毁缓冲区
            buffer.destroy(m_device);
            buffer.solidVertexBuffer = VK_NULL_HANDLE;
            buffer.solidVertexMemory = VK_NULL_HANDLE;
            buffer.solidIndexBuffer = VK_NULL_HANDLE;
            buffer.solidIndexMemory = VK_NULL_HANDLE;
            buffer.solidVertexCount = 0;
            buffer.solidIndexCount = 0;
        }

        // 更新统计（实心）
        if (m_totalSolidVertices >= oldSolidVertices) {
            m_totalSolidVertices -= oldSolidVertices;
        } else {
            m_totalSolidVertices = 0;
        }
        if (m_totalSolidIndices >= oldSolidIndices) {
            m_totalSolidIndices -= oldSolidIndices;
        } else {
            m_totalSolidIndices = 0;
        }
        m_totalSolidVertices += buffer.solidVertexCount;
        m_totalSolidIndices += buffer.solidIndexCount;

        // 更新透明网格
        u32 oldTransparentVertices = buffer.transparentVertexCount;
        u32 oldTransparentIndices = buffer.transparentIndexCount;

        if (!transparentMesh.empty()) {
            auto result = createMeshBuffers(
                buffer.transparentVertexBuffer,
                buffer.transparentVertexMemory,
                buffer.transparentIndexBuffer,
                buffer.transparentIndexMemory,
                transparentMesh,
                buffer.transparentVertexCount,
                buffer.transparentIndexCount);
            if (!result.success()) {
                return result;
            }
            buffer.hasTransparentMesh = true;
        } else if (buffer.transparentVertexBuffer != VK_NULL_HANDLE) {
            // 透明网格已清空，销毁缓冲区
            vkDestroyBuffer(m_device, buffer.transparentVertexBuffer, nullptr);
            vkFreeMemory(m_device, buffer.transparentVertexMemory, nullptr);
            vkDestroyBuffer(m_device, buffer.transparentIndexBuffer, nullptr);
            vkFreeMemory(m_device, buffer.transparentIndexMemory, nullptr);
            buffer.transparentVertexBuffer = VK_NULL_HANDLE;
            buffer.transparentVertexMemory = VK_NULL_HANDLE;
            buffer.transparentIndexBuffer = VK_NULL_HANDLE;
            buffer.transparentIndexMemory = VK_NULL_HANDLE;
            buffer.transparentVertexCount = 0;
            buffer.transparentIndexCount = 0;
            buffer.hasTransparentMesh = false;
        }

        // 更新统计（透明）
        if (m_totalTransparentVertices >= oldTransparentVertices) {
            m_totalTransparentVertices -= oldTransparentVertices;
        } else {
            m_totalTransparentVertices = 0;
        }
        if (m_totalTransparentIndices >= oldTransparentIndices) {
            m_totalTransparentIndices -= oldTransparentIndices;
        } else {
            m_totalTransparentIndices = 0;
        }
        m_totalTransparentVertices += buffer.transparentVertexCount;
        m_totalTransparentIndices += buffer.transparentIndexCount;

        buffer.isValid = (buffer.solidVertexBuffer != VK_NULL_HANDLE ||
                          buffer.transparentVertexBuffer != VK_NULL_HANDLE);
    }

    return {};
}

void ChunkRenderer::removeChunk(const ChunkId& chunkId) {
    u64 id = chunkId.toId();
    auto it = m_chunkBuffers.find(id);

    if (it != m_chunkBuffers.end()) {
        // 更新统计
        m_totalSolidVertices -= it->second->solidVertexCount;
        m_totalSolidIndices -= it->second->solidIndexCount;
        m_totalTransparentVertices -= it->second->transparentVertexCount;
        m_totalTransparentIndices -= it->second->transparentIndexCount;

        // 将缓冲区移入延迟销毁队列
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

void ChunkRenderer::clearChunks() {
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
    m_totalSolidVertices = 0;
    m_totalSolidIndices = 0;
    m_totalTransparentVertices = 0;
    m_totalTransparentIndices = 0;
}

Result<void> ChunkRenderer::createMeshBuffers(
    VkBuffer& vertexBuffer,
    VkDeviceMemory& vertexMemory,
    VkBuffer& indexBuffer,
    VkDeviceMemory& indexMemory,
    const MeshData& meshData,
    u32& outVertexCount,
    u32& outIndexCount)
{
    const u32 oldVertexCount = outVertexCount;
    const u32 oldIndexCount = outIndexCount;

    VkDeviceSize vertexSize = static_cast<VkDeviceSize>(meshData.vertices.size() * sizeof(Vertex));
    VkDeviceSize indexSize = static_cast<VkDeviceSize>(meshData.indices.size() * sizeof(u32));

    // 如果缓冲区已存在且大小足够，重用
    bool needNewVertex = vertexBuffer == VK_NULL_HANDLE || outVertexCount < meshData.vertices.size();
    bool needNewIndex = indexBuffer == VK_NULL_HANDLE || outIndexCount < meshData.indices.size();

    // 创建顶点缓冲区
    if (needNewVertex) {
        if (vertexBuffer != VK_NULL_HANDLE) {
            // 延迟销毁旧缓冲区，避免 GPU 仍在使用时被提前释放导致 device lost
            auto oldBuffer = std::make_unique<ChunkGpuBuffer>();
            oldBuffer->solidVertexBuffer = vertexBuffer;
            oldBuffer->solidVertexMemory = vertexMemory;
            oldBuffer->solidVertexCount = oldVertexCount;
            oldBuffer->isValid = true;

            {
                std::lock_guard<std::mutex> lock(m_pendingDestroysMutex);
                PendingBufferDestroy pending;
                pending.buffer = std::move(oldBuffer);
                pending.frameIndex = m_destroyFrameCounter;
                m_pendingDestroys.push_back(std::move(pending));
            }

            vertexBuffer = VK_NULL_HANDLE;
            vertexMemory = VK_NULL_HANDLE;
        }

        auto result = createBuffer(
            vertexSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vertexBuffer,
            vertexMemory);

        if (result.failed()) {
            return Error(ErrorCode::InitializationFailed,
                "Failed to create vertex buffer: " + result.error().message());
        }
    }

    // 创建索引缓冲区
    if (needNewIndex) {
        if (indexBuffer != VK_NULL_HANDLE) {
            // 延迟销毁旧缓冲区
            auto oldBuffer = std::make_unique<ChunkGpuBuffer>();
            oldBuffer->solidIndexBuffer = indexBuffer;
            oldBuffer->solidIndexMemory = indexMemory;
            oldBuffer->solidIndexCount = oldIndexCount;
            oldBuffer->isValid = true;

            {
                std::lock_guard<std::mutex> lock(m_pendingDestroysMutex);
                PendingBufferDestroy pending;
                pending.buffer = std::move(oldBuffer);
                pending.frameIndex = m_destroyFrameCounter;
                m_pendingDestroys.push_back(std::move(pending));
            }

            indexBuffer = VK_NULL_HANDLE;
            indexMemory = VK_NULL_HANDLE;
        }

        auto result = createBuffer(
            indexSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            indexBuffer,
            indexMemory);

        if (result.failed()) {
            return Error(ErrorCode::InitializationFailed,
                "Failed to create index buffer: " + result.error().message());
        }
    }

    // 上传数据
    auto cmdResult = beginSingleTimeCommands();
    if (!cmdResult.success()) {
        return cmdResult.error();
    }
    VkCommandBuffer commandBuffer = cmdResult.value();

    // 确保暂存缓冲区足够大
    VkDeviceSize maxDataSize = std::max(vertexSize, indexSize);
    if (maxDataSize > m_stagingBufferSize || m_stagingBuffer == VK_NULL_HANDLE) {
        // 销毁旧的暂存缓冲区
        if (m_stagingBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, m_stagingBuffer, nullptr);
            vkFreeMemory(m_device, m_stagingMemory, nullptr);
        }

        m_stagingBufferSize = std::max(maxDataSize, static_cast<VkDeviceSize>(16 * 1024 * 1024));
        auto result = createBuffer(
            m_stagingBufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_stagingBuffer,
            m_stagingMemory);

        if (result.failed()) {
            endSingleTimeCommands(commandBuffer);
            return Error(ErrorCode::OperationFailed, "Failed to create staging buffer");
        }
    }

    // 映射并上传顶点数据
    void* mapped;
    vkMapMemory(m_device, m_stagingMemory, 0, vertexSize, 0, &mapped);
    std::memcpy(mapped, meshData.vertices.data(), static_cast<size_t>(vertexSize));
    vkUnmapMemory(m_device, m_stagingMemory);

    // 复制顶点数据
    VkBufferCopy copyRegion{};
    copyRegion.size = vertexSize;
    vkCmdCopyBuffer(commandBuffer, m_stagingBuffer, vertexBuffer, 1, &copyRegion);

    // 映射并上传索引数据
    vkMapMemory(m_device, m_stagingMemory, 0, indexSize, 0, &mapped);
    std::memcpy(mapped, meshData.indices.data(), static_cast<size_t>(indexSize));
    vkUnmapMemory(m_device, m_stagingMemory);

    // 复制索引数据
    copyRegion.size = indexSize;
    vkCmdCopyBuffer(commandBuffer, m_stagingBuffer, indexBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);

    outIndexCount = static_cast<u32>(meshData.indices.size());
    outVertexCount = static_cast<u32>(meshData.vertices.size());

    return {};
}

Result<void> ChunkRenderer::loadTextureAtlas(
    const u8* pixelData,
    u32 width,
    u32 height,
    u32 tileSize)
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
    auto result = createTextureAtlas(width, height);
    if (result.failed()) {
        return result;
    }

    m_textureAtlas.tileSize = tileSize;
    m_textureAtlas.tilesPerRow = width / tileSize;
    m_textureAtlas.tileU = static_cast<f32>(tileSize) / static_cast<f32>(width);
    m_textureAtlas.tileV = static_cast<f32>(tileSize) / static_cast<f32>(height);

    // 上传纹理数据
    return uploadTextureData(pixelData, width, height);
}

Result<void> ChunkRenderer::createTextureAtlas(u32 width, u32 height) {
    // 销毁旧纹理
    m_textureAtlas.destroy(m_device);

    // 创建图像
    auto imageResult = renderer::VulkanUtils::createImage(
        m_device, m_physicalDevice,
        width, height,
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
        m_device, m_textureAtlas.image,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT,
        m_textureAtlas.imageView);

    if (viewResult.failed()) {
        m_textureAtlas.destroy(m_device);
        return viewResult.error();
    }

    // 创建采样器
    auto samplerResult = renderer::VulkanUtils::createSampler(
        m_device,
        VK_FILTER_NEAREST, VK_FILTER_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
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

Result<void> ChunkRenderer::uploadTextureData(const u8* pixelData, u32 width, u32 height) {
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

    auto result = createBuffer(
        imageSize,
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
    auto cmdResult = beginSingleTimeCommands();
    if (cmdResult.failed()) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
        return cmdResult.error();
    }
    VkCommandBuffer cmd = cmdResult.value();

    // 转换到传输目标布局
    renderer::VulkanUtils::transitionImageLayout(
        cmd, m_textureAtlas.image,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    // 复制缓冲区到图像
    renderer::VulkanUtils::copyBufferToImage(cmd, stagingBuffer, m_textureAtlas.image, width, height);

    // 转换到着色器只读布局
    renderer::VulkanUtils::transitionImageLayout(
        cmd, m_textureAtlas.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    endSingleTimeCommands(cmd);

    // 清理暂存缓冲区
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingMemory, nullptr);

    return {};
}

// ============================================================================
// 实心网格渲染
// ============================================================================

void ChunkRenderer::renderSolid(VkCommandBuffer commandBuffer, VkPipelineLayout /*pipelineLayout*/) {
    // 渲染所有实心方块
    for (const auto& pair : m_chunkBuffers) {
        const auto& buffer = pair.second;
        if (!buffer->isValid || buffer->solidVertexBuffer == VK_NULL_HANDLE || buffer->solidIndexBuffer == VK_NULL_HANDLE) {
            continue;
        }

        VkBuffer vertexBuffers[] = { buffer->solidVertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, buffer->solidIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(
            commandBuffer,
            buffer->solidIndexCount,
            1,  // instance count
            0,  // first index
            0,  // vertex offset
            0   // first instance
        );
    }
}

void ChunkRenderer::renderSolid(VkCommandBuffer commandBuffer, VkPipelineLayout /*pipelineLayout*/,
                                PushConstantsCallback pushConstantsCallback) {
    // 渲染所有实心方块
    for (const auto& pair : m_chunkBuffers) {
        const auto& buffer = pair.second;
        if (!buffer->isValid || buffer->solidVertexBuffer == VK_NULL_HANDLE || buffer->solidIndexBuffer == VK_NULL_HANDLE) {
            continue;
        }

        // 调用回调设置推送常量（区块偏移）
        if (pushConstantsCallback) {
            pushConstantsCallback(buffer->chunkId);
        }

        VkBuffer vertexBuffers[] = { buffer->solidVertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, buffer->solidIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(
            commandBuffer,
            buffer->solidIndexCount,
            1,  // instance count
            0,  // first index
            0,  // vertex offset
            0   // first instance
        );
    }
}

// ============================================================================
// 透明网格渲染
// ============================================================================

std::vector<ChunkId> ChunkRenderer::sortChunksByDistance(const glm::vec3& cameraPosition) const {
    std::vector<std::pair<ChunkId, float>> distances;
    distances.reserve(m_chunkBuffers.size());

    for (const auto& pair : m_chunkBuffers) {
        const auto& buffer = pair.second;
        if (!buffer->isValid || !buffer->hasTransparentMesh) {
            continue;
        }

        float distance = glm::length(buffer->centerPosition - cameraPosition);
        distances.emplace_back(buffer->chunkId, distance);
    }

    // 从远到近排序（透明物体需要从后往前渲染）
    std::sort(distances.begin(), distances.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

    std::vector<ChunkId> result;
    result.reserve(distances.size());
    for (const auto& p : distances) {
        result.push_back(p.first);
    }

    return result;
}

void ChunkRenderer::renderTransparent(VkCommandBuffer commandBuffer, VkPipelineLayout /*pipelineLayout*/,
                                      const glm::vec3& cameraPosition) {
    // 按距离排序透明区块
    auto sortedChunks = sortChunksByDistance(cameraPosition);

    // 从远到近渲染
    for (const auto& chunkId : sortedChunks) {
        u64 id = chunkId.toId();
        auto it = m_chunkBuffers.find(id);
        if (it == m_chunkBuffers.end()) {
            continue;
        }

        const auto& buffer = it->second;
        if (!buffer->isValid || !buffer->hasTransparentMesh ||
            buffer->transparentVertexBuffer == VK_NULL_HANDLE ||
            buffer->transparentIndexBuffer == VK_NULL_HANDLE) {
            continue;
        }

        VkBuffer vertexBuffers[] = { buffer->transparentVertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, buffer->transparentIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(
            commandBuffer,
            buffer->transparentIndexCount,
            1,  // instance count
            0,  // first index
            0,  // vertex offset
            0   // first instance
        );
    }
}

void ChunkRenderer::renderTransparent(VkCommandBuffer commandBuffer, VkPipelineLayout /*pipelineLayout*/,
                                      const glm::vec3& cameraPosition,
                                      PushConstantsCallback pushConstantsCallback) {
    // 按距离排序透明区块
    auto sortedChunks = sortChunksByDistance(cameraPosition);

    // 从远到近渲染
    for (const auto& chunkId : sortedChunks) {
        u64 id = chunkId.toId();
        auto it = m_chunkBuffers.find(id);
        if (it == m_chunkBuffers.end()) {
            continue;
        }

        const auto& buffer = it->second;
        if (!buffer->isValid || !buffer->hasTransparentMesh ||
            buffer->transparentVertexBuffer == VK_NULL_HANDLE ||
            buffer->transparentIndexBuffer == VK_NULL_HANDLE) {
            continue;
        }

        // 调用回调设置推送常量（区块偏移）
        if (pushConstantsCallback) {
            pushConstantsCallback(buffer->chunkId);
        }

        VkBuffer vertexBuffers[] = { buffer->transparentVertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, buffer->transparentIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(
            commandBuffer,
            buffer->transparentIndexCount,
            1,  // instance count
            0,  // first index
            0,  // vertex offset
            0   // first instance
        );
    }
}

// ============================================================================
// 向后兼容接口
// ============================================================================

void ChunkRenderer::render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    renderSolid(commandBuffer, pipelineLayout);
}

void ChunkRenderer::render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                           PushConstantsCallback pushConstantsCallback) {
    renderSolid(commandBuffer, pipelineLayout, pushConstantsCallback);
}

Result<VkCommandBuffer> ChunkRenderer::beginSingleTimeCommands() {
    VkCommandBuffer cmd = renderer::VulkanUtils::beginSingleTimeCommands(m_device, m_commandPool);
    if (cmd == VK_NULL_HANDLE) {
        return Error(ErrorCode::OperationFailed, "Failed to allocate command buffer");
    }
    return cmd;
}

void ChunkRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    // 使用 fence 版本，避免阻塞整个 GPU 队列
    renderer::VulkanUtils::endSingleTimeCommands(m_device, m_commandPool, m_graphicsQueue, commandBuffer);
}

// ============================================================================
// 异步 GPU 上传
// ============================================================================

void ChunkRenderer::submitMeshUpload(const ChunkId& chunkId, MeshData&& meshData) {
    PendingMeshUpload upload;
    upload.chunkId = chunkId;
    upload.meshData = std::move(meshData);
    upload.submitTime = m_uploadTimestamp++;

    std::lock_guard<std::mutex> lock(m_pendingMutex);
    m_pendingUploads.push(std::move(upload));
}

u32 ChunkRenderer::processPendingUploads(u32 maxPerFrame) {
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
                         upload.chunkId.x, upload.chunkId.z, result.error().message());
        }
    }

    return processed;
}

size_t ChunkRenderer::pendingUploadCount() const {
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    return m_pendingUploads.size();
}

void ChunkRenderer::processPendingDestroys(u32 framesToKeep) {
    std::lock_guard<std::mutex> lock(m_pendingDestroysMutex);

    // 递增帧计数器
    u64 currentCounter = m_destroyFrameCounter++;

    // 销毁超过保留帧数的缓冲区
    auto it = m_pendingDestroys.begin();
    while (it != m_pendingDestroys.end()) {
        u64 frameDiff = currentCounter >= it->frameIndex
            ? currentCounter - it->frameIndex
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

void FenceManager::cleanup(VkDevice device, VkCommandPool commandPool) {
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

void FenceManager::destroy(VkDevice device, VkCommandPool commandPool) {
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
