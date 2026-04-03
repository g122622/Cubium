#pragma once

#include "../../../../common/core/Types.hpp"
#include "../../../../common/core/Result.hpp"
#include "../../MeshTypes.hpp"
#include "ChunkMesher.hpp"
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <memory>
#include <vector>
#include <mutex>
#include <queue>
#include <functional>
#include <glm/vec3.hpp>

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
    ChunkId chunkId{0, 0};
    ChunkRenderLayer layer = ChunkRenderLayer::Solid;
    bool isValid = false;

    void destroy(VkDevice device);
};

// 待上传的网格数据
struct PendingMeshUpload {
    ChunkId chunkId;
    MeshData meshData;
    u64 submitTime = 0;  // 提交时间戳（用于超时检测）
};

// 待销毁的缓冲区（用于延迟销毁）
struct PendingBufferDestroy {
    std::unique_ptr<ChunkGpuBuffer> buffer;
    u64 frameIndex;  // 创建时的帧号，用于计算延迟销毁
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

// 纹理图集 - 使用原始 Vulkan handles
struct ChunkTextureAtlas {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    u32 width = 0;
    u32 height = 0;
    u32 tileSize = 16;
    u32 tilesPerRow = 0;
    f64 tileU = 0.0f;
    f64 tileV = 0.0f;
    bool isValid = false;

    void destroy(VkDevice device);

    [[nodiscard]] TextureRegion getRegion(u32 tileX, u32 tileY) const;
    [[nodiscard]] TextureRegion getRegion(u32 tileIndex) const;
};

// 区块渲染器
class ChunkRenderer {
public:
    struct StagingCopyLayout {
        VkDeviceSize vertexOffset = 0;
        VkDeviceSize indexOffset = 0;
        VkDeviceSize totalSize = 0;
    };

    [[nodiscard]] static constexpr VkDeviceSize stagingCopyAlignment() {
        return 4;
    }

    [[nodiscard]] static constexpr VkDeviceSize alignStagingOffset(
        VkDeviceSize value,
        VkDeviceSize alignment)
    {
        return alignment == 0
            ? value
            : ((value + alignment - 1) / alignment) * alignment;
    }

    [[nodiscard]] static constexpr StagingCopyLayout buildStagingCopyLayout(
        VkDeviceSize vertexSize,
        VkDeviceSize indexSize)
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
    [[nodiscard]] Result<void> initialize(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        u32 maxChunks = 8192);

    void destroy();

    // 区块管理
    [[nodiscard]] Result<void> updateChunk(
        const ChunkId& chunkId,
        const MeshData& meshData);

    /**
     * @brief 更新区块双层网格
     *
     * @param chunkId 区块 ID
     * @param solidMesh 实心网格
     * @param transparentMesh 半透明网格
     */
    [[nodiscard]] Result<void> updateChunk(
        const ChunkId& chunkId,
        const MeshData& solidMesh,
        const MeshData& transparentMesh);

    void removeChunk(const ChunkId& chunkId);

    void clearChunks();

    // 纹理图集
    [[nodiscard]] Result<void> loadTextureAtlas(
        const u8* pixelData,
        u32 width,
        u32 height,
        u32 tileSize);

    ChunkTextureAtlas& textureAtlas() { return m_textureAtlas; }
    const ChunkTextureAtlas& textureAtlas() const { return m_textureAtlas; }

    // 渲染
    void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

    // 渲染（带推送常量回调）
    // pushConstantsCallback: 设置推送常量的回调函数，参数是 chunkId
    using PushConstantsCallback = std::function<void(const ChunkId&)>;
    void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                PushConstantsCallback pushConstantsCallback);

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

    // ========== 异步 GPU 上传 ==========

    /**
     * @brief 提交网格上传请求
     *
     * 将网格数据添加到待上传队列，非阻塞调用。
     * 实际上传在 processPendingUploads() 中执行。
     *
     * @param chunkId 区块 ID
     * @param meshData 网格数据（会被移动）
     */
    void submitMeshUpload(const ChunkId& chunkId, MeshData&& meshData);

    /**
     * @brief 处理待上传的网格数据
     *
     * 每帧调用一次，处理最多 maxPerFrame 个待上传网格。
     * 使用 fence 实现非阻塞上传，避免 GPU 等待。
     *
     * @param maxPerFrame 每帧最多处理数量（默认 4）
     * @return 实际处理的数量
     */
    u32 processPendingUploads(u32 maxPerFrame = 4);

    /**
     * @brief 获取待上传队列大小
     */
    [[nodiscard]] size_t pendingUploadCount() const;

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
    [[nodiscard]] static u64 makeBufferKey(const ChunkId& chunkId, ChunkRenderLayer layer) {
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

    // 纹理图集
    ChunkTextureAtlas m_textureAtlas;

    // 暂存缓冲区
    VkBuffer m_stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_stagingMemory = VK_NULL_HANDLE;
    VkDeviceSize m_stagingBufferSize = 16 * 1024 * 1024; // 16MB
    void* m_stagingMapped = nullptr;

    // ========== 异步上传支持 ==========

    // 待上传队列
    std::queue<PendingMeshUpload> m_pendingUploads;
    mutable std::mutex m_pendingMutex;
    u64 m_uploadTimestamp = 0;

    // Fence 管理（用于非阻塞上传）
    FenceManager m_fenceManager;
    static constexpr u32 MAX_IN_FLIGHT_UPLOADS = 8;  // 最大同时上传数量

    // 延迟销毁队列
    std::vector<PendingBufferDestroy> m_pendingDestroys;
    mutable std::mutex m_pendingDestroysMutex;
    u64 m_destroyFrameCounter = 0;  // 每次调用 processPendingDestroys 递增

    // 单次命令缓冲区
    [[nodiscard]] Result<VkCommandBuffer> beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    // 创建缓冲区
    [[nodiscard]] Result<void> createBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& memory);

    // 创建/更新缓冲区
    [[nodiscard]] Result<void> createChunkBuffer(
        ChunkGpuBuffer& buffer,
        const MeshData& meshData);

    [[nodiscard]] Result<void> updateChunkLayer(
        const ChunkId& chunkId,
        const MeshData& meshData,
        ChunkRenderLayer layer);

    // 上传缓冲区数据
    [[nodiscard]] Result<void> uploadBufferData(
        VkBuffer dstBuffer,
        const void* data,
        VkDeviceSize size);

    // 查找内存类型
    [[nodiscard]] Result<u32> findMemoryType(
        u32 typeFilter,
        VkMemoryPropertyFlags properties);

    // 创建纹理图集
    [[nodiscard]] Result<void> createTextureAtlas(
        u32 width,
        u32 height);

    // 上传纹理数据
    [[nodiscard]] Result<void> uploadTextureData(
        const u8* pixelData,
        u32 width,
        u32 height);
};

} // namespace mc::client
