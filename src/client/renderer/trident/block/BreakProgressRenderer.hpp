#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace mc {
class ResourceManager;

namespace client {
namespace renderer {

// 前向声明（在 renderer 命名空间）
class DestroyStageTextures;

namespace trident {
namespace block {

// 前向声明（在 trident::block 命名空间）
class BreakProgressManager;

/**
 * @brief 破坏纹理渲染器
 *
 * 负责在方块上渲染破坏进度覆盖层。使用特殊的叠加混合模式
 * (DST_COLOR * SRC_COLOR) 将破坏纹理叠加到方块表面。
 *
 * 渲染流程：
 * 1. 从 BreakProgressManager 获取所有可见的破坏进度
 * 2. 为每个方块生成一个覆盖层立方体网格
 * 3. 使用破坏纹理图集采样对应阶段的纹理
 * 4. 通过叠加混合渲染到场景中
 *
 * 参考 MC 1.16.5 WorldRenderer.renderBlockBreakProgress()
 */
class BreakProgressRenderer {
public:
    /**
     * @brief 顶点格式
     */
    struct Vertex {
        f32 x, y, z;       // 位置
        f32 u, v;          // UV坐标
    };

    /**
     * @brief 配置参数
     */
    struct Config {
        VkDevice device = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkDescriptorSetLayout cameraLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout fogLayout = VK_NULL_HANDLE;
        u32 maxFramesInFlight = 2;
        ResourceManager* resourceManager = nullptr;  ///< 资源管理器（用于加载破坏纹理）
    };

    /**
     * @brief 默认构造函数
     */
    BreakProgressRenderer() = default;

    /**
     * @brief 析构函数
     */
    ~BreakProgressRenderer();

    // 禁止拷贝
    BreakProgressRenderer(const BreakProgressRenderer&) = delete;
    BreakProgressRenderer& operator=(const BreakProgressRenderer&) = delete;

    /**
     * @brief 初始化渲染器
     *
     * 创建Vulkan资源：管线、缓冲区、纹理等。
     *
     * @param config 配置参数
     * @return 成功返回 true
     */
    [[nodiscard]] bool initialize(const Config& config);

    /**
     * @brief 清理资源
     */
    void cleanup();

    /**
     * @brief 更新破坏进度网格
     *
     * 从 BreakProgressManager 获取所有可见的破坏进度，
     * 更新顶点缓冲区。
     *
     * @param cameraPos 摄像机位置（用于距离裁剪）
     */
    void updateMesh(const Vector3& cameraPos);

    /**
     * @brief 渲染破坏覆盖层
     *
     * 使用叠加混合模式渲染所有破坏进度。
     * 应在区块渲染之后、GUI渲染之前调用。
     *
     * @param commandBuffer Vulkan命令缓冲区
     * @param cameraDescriptorSet 相机描述符集
     * @param fogDescriptorSet 雾效果描述符集
     */
    void render(VkCommandBuffer commandBuffer,
                VkDescriptorSet cameraDescriptorSet,
                VkDescriptorSet fogDescriptorSet);

    /**
     * @brief 检查是否有破坏进度需要渲染
     */
    [[nodiscard]] bool hasProgressToRender() const { return !m_progressEntries.empty(); }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    /**
     * @brief 破坏进度条目（用于渲染）
     */
    struct ProgressEntry {
        BlockPos position;  // 方块位置
        u8 stage;           // 破坏阶段 (0-9)
        u32 vertexOffset;   // 该方块在顶点缓冲区中的起始偏移
        u32 indexOffset;    // 该方块在索引缓冲区中的起始偏移（以索引为单位）
    };

    /**
     * @brief 创建管线
     */
    [[nodiscard]] bool createPipeline();

    /**
     * @brief 创建顶点/索引缓冲区
     */
    [[nodiscard]] bool createBuffers();

    /**
     * @brief 创建描述符集
     */
    [[nodiscard]] bool createDescriptorSets();

    /**
     * @brief 上传破坏纹理图集到GPU
     */
    [[nodiscard]] bool uploadTextureAtlas();

    /**
     * @brief 生成立方体顶点数据
     *
     * 生成一个稍大于1的立方体（防止z-fighting）
     * 使用局部坐标（0-1范围），方块位置通过push constants传入
     *
     * @param cubeIndex 立方体索引（用于计算索引偏移）
     * @param vertices 输出顶点数组
     * @param indices 输出索引数组
     */
    void generateCubeMesh(size_t cubeIndex,
                          std::vector<Vertex>& vertices,
                          std::vector<u32>& indices);

    /**
     * @brief 确保缓冲区容量足够
     *
     * 如果当前缓冲区容量不足，重新创建更大的缓冲区。
     *
     * @param requiredVertices 所需顶点容量
     * @param requiredIndices 所需索引容量
     * @return 成功返回 true
     */
    [[nodiscard]] bool ensureBufferCapacity(size_t requiredVertices, size_t requiredIndices);

    /**
     * @brief 重新创建缓冲区
     *
     * @param vertexCount 新的顶点容量
     * @param indexCount 新的索引容量
     * @return 成功返回 true
     */
    [[nodiscard]] bool recreateBuffers(size_t vertexCount, size_t indexCount);

    /**
     * @brief 更新顶点缓冲区
     */
    void updateVertexBuffer(const std::vector<Vertex>& vertices);

    /**
     * @brief 更新索引缓冲区
     */
    void updateIndexBuffer(const std::vector<u32>& indices);

    // ========================================================================
    // 配置参数
    // ========================================================================

    Config m_config;

    // ========================================================================
    // Vulkan 资源
    // ========================================================================

    /// 管线布局
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

    /// 图形管线
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    /// 描述符集布局（纹理）
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

    /// 描述符池
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    /// 描述符集（纹理）
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    /// 顶点缓冲区
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;

    /// 顶点缓冲区内存
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;

    /// 索引缓冲区
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;

    /// 索引缓冲区内存
    VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;

    /// 破坏纹理图集
    VkImage m_textureImage = VK_NULL_HANDLE;
    VkDeviceMemory m_textureImageMemory = VK_NULL_HANDLE;
    VkImageView m_textureImageView = VK_NULL_HANDLE;
    VkSampler m_textureSampler = VK_NULL_HANDLE;

    /// 暂存缓冲区（用于纹理上传）
    VkBuffer m_stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_stagingBufferMemory = VK_NULL_HANDLE;

    // ========================================================================
    // 渲染数据
    // ========================================================================

    /// 当前帧的破坏进度条目
    std::vector<ProgressEntry> m_progressEntries;

    /// 预分配的进度查询缓冲区（避免每帧分配）
    std::vector<std::pair<BlockPos, u8>> m_progressBuffer;

    /// 顶点数量
    size_t m_vertexCount = 0;

    /// 索引数量
    size_t m_indexCount = 0;

    /// 缓冲区最大容量
    size_t m_maxVertices = 0;
    size_t m_maxIndices = 0;

    /// 是否已初始化
    bool m_initialized = false;
};

} // namespace block
} // namespace trident
} // namespace renderer
} // namespace client
} // namespace mc
