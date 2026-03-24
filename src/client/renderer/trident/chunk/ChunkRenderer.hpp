#pragma once

#include "../../../../common/core/Types.hpp"
#include "../../../../common/core/Result.hpp"
#include "../../MeshTypes.hpp"
#include "ChunkMesher.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <memory>
#include <vector>
#include <mutex>
#include <queue>
#include <functional>

namespace mc::client {

// 区块GPU缓冲区 - 使用原始 Vulkan handles（支持实心+透明双网格）
struct ChunkGpuBuffer {
    // 实心网格缓冲区
    VkBuffer solidVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory solidVertexMemory = VK_NULL_HANDLE;
    VkBuffer solidIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory solidIndexMemory = VK_NULL_HANDLE;
    u32 solidIndexCount = 0;
    u32 solidVertexCount = 0;

    // 透明网格缓冲区
    VkBuffer transparentVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory transparentVertexMemory = VK_NULL_HANDLE;
    VkBuffer transparentIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory transparentIndexMemory = VK_NULL_HANDLE;
    u32 transparentIndexCount = 0;
    u32 transparentVertexCount = 0;

    ChunkId chunkId{0, 0};
    glm::vec3 centerPosition{0.0f};  // 用于距离排序
    bool isValid = false;
    bool hasTransparentMesh = false;

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
    f32 tileU = 0.0f;
    f32 tileV = 0.0f;
    bool isValid = false;

    void destroy(VkDevice device);

    [[nodiscard]] TextureRegion getRegion(u32 tileX, u32 tileY) const;
    [[nodiscard]] TextureRegion getRegion(u32 tileIndex) const;
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
    [[nodiscard]] Result<void> initialize(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        u32 maxChunks = 1024);

    void destroy();

    // 区块管理
    /**
     * @brief 更新区块网格（仅实心网格）
     *
     * 向后兼容的接口，透明网格为空。
     */
    [[nodiscard]] Result<void> updateChunk(
        const ChunkId& chunkId,
        const MeshData& meshData);

    /**
     * @brief 更新区块网格（实心+透明双网格）
     *
     * @param chunkId 区块 ID
     * @param solidMesh 实心方块网格
     * @param transparentMesh 透明方块网格（水、玻璃等）
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

    // ========== 实心网格渲染 ==========

    /**
     * @brief 渲染实心方块网格
     *
     * 渲染所有区块的实心网格，用于不透明渲染通道。
     *
     * @param commandBuffer 命令缓冲区
     * @param pipelineLayout 管线布局
     */
    void renderSolid(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

    /**
     * @brief 渲染实心网格（带推送常量回调）
     */
    using PushConstantsCallback = std::function<void(const ChunkId&)>;
    void renderSolid(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                     PushConstantsCallback pushConstantsCallback);

    // ========== 透明网格渲染 ==========

    /**
     * @brief 渲染透明方块网格
     *
     * 按距离从远到近排序后渲染所有透明区块。
     * 用于透明渲染通道（水、玻璃等）。
     *
     * @param commandBuffer 命令缓冲区
     * @param pipelineLayout 管线布局
     * @param cameraPosition 相机位置（用于排序）
     */
    void renderTransparent(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                           const glm::vec3& cameraPosition);

    /**
     * @brief 渲染透明网格（带推送常量回调）
     */
    void renderTransparent(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                           const glm::vec3& cameraPosition,
                           PushConstantsCallback pushConstantsCallback);

    // ========== 向后兼容接口 ==========

    /**
     * @brief 渲染所有区块（向后兼容）
     * @deprecated 使用 renderSolid() 替代
     */
    void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

    /**
     * @brief 渲染所有区块（带推送常量，向后兼容）
     * @deprecated 使用 renderSolid() 替代
     */
    void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                PushConstantsCallback pushConstantsCallback);

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
    u32 totalSolidVertexCount() const { return m_totalSolidVertices; }
    u32 totalSolidIndexCount() const { return m_totalSolidIndices; }
    u32 totalTransparentVertexCount() const { return m_totalTransparentVertices; }
    u32 totalTransparentIndexCount() const { return m_totalTransparentIndices; }
    u32 totalVertexCount() const { return m_totalSolidVertices + m_totalTransparentVertices; }
    u32 totalIndexCount() const { return m_totalSolidIndices + m_totalTransparentIndices; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;

    // 区块缓冲区
    std::unordered_map<u64, std::unique_ptr<ChunkGpuBuffer>> m_chunkBuffers;
    u32 m_maxChunks = 1024;

    // 统计
    u32 m_totalSolidVertices = 0;
    u32 m_totalSolidIndices = 0;
    u32 m_totalTransparentVertices = 0;
    u32 m_totalTransparentIndices = 0;

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

    // ========== 透明区块排序 ==========

    /**
     * @brief 对透明区块按距离排序
     *
     * @param cameraPosition 相机位置
     * @return 排序后的区块ID列表（从远到近）
     */
    [[nodiscard]] std::vector<ChunkId> sortChunksByDistance(const glm::vec3& cameraPosition) const;

    // ========== 缓冲区创建辅助 ==========

    /**
     * @brief 创建单个网格的 GPU 缓冲区
     *
     * @param buffer 目标缓冲区
     * @param meshData 网格数据
     * @param[out] outVertexCount 输出顶点数
     * @param[out] outIndexCount 输出索引数
     * @return 结果
     */
    [[nodiscard]] Result<void> createMeshBuffers(
        VkBuffer& vertexBuffer,
        VkDeviceMemory& vertexMemory,
        VkBuffer& indexBuffer,
        VkDeviceMemory& indexMemory,
        const MeshData& meshData,
        u32& outVertexCount,
        u32& outIndexCount);
};

} // namespace mc::client
