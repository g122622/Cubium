#pragma once

#include "Particle.hpp"
#include "ParticleTextureAtlas.hpp"
#include "common/core/Result.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <array>
#include <unordered_map>

namespace mc::client::renderer::trident::particle {

/**
 * @brief 粒子管理器 Uniform 缓冲区数据结构
 */
struct ParticleUBO {
    alignas(16) glm::mat4 projection;   ///< 投影矩阵
    alignas(16) glm::mat4 view;         ///< 视图矩阵
    alignas(16) glm::vec3 cameraPos;    ///< 相机位置
    alignas(4) f32 partialTick;         ///< 部分 tick
};

/**
 * @brief 粒子管理器
 *
 * 管理所有粒子的生命周期、更新和渲染。
 * 参考 MC 1.16.5 ParticleManager
 *
 * 功能：
 * - 粒子的创建和销毁
 * - 按渲染类型分组管理
 * - GPU 缓冲区管理
 * - Vulkan 渲染管线
 * - 纹理图集支持
 */
class ParticleManager {
public:
    ParticleManager();
    ~ParticleManager();

    // 禁止拷贝
    ParticleManager(const ParticleManager&) = delete;
    ParticleManager& operator=(const ParticleManager&) = delete;

    // ========================================================================
    // 初始化与销毁
    // ========================================================================

    /**
     * @brief 初始化粒子管理器
     *
     * @param device Vulkan 逻辑设备
     * @param physicalDevice Vulkan 物理设备
     * @param commandPool 命令池
     * @param graphicsQueue 图形队列
     * @param renderPass 渲染通道
     * @param extent 交换链图像尺寸
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        VkRenderPass renderPass,
        VkExtent2D extent,
        VkSampleCountFlagBits sampleCount);

    /**
     * @brief 销毁所有资源
     */
    void destroy();

    /**
     * @brief 窗口大小变化时重新创建资源
     */
    [[nodiscard]] Result<void> onResize(VkExtent2D extent);

    // ========================================================================
    // 粒子管理
    // ========================================================================

    /**
     * @brief 添加粒子
     *
     * @param particle 粒子实例
     */
    void addParticle(std::unique_ptr<Particle> particle);

    /**
     * @brief 清除所有粒子
     */
    void clear();

    /**
     * @brief 获取粒子数量
     */
    [[nodiscard]] size_t particleCount() const { return m_particles.size(); }

    /**
     * @brief 获取存活的粒子数量
     */
    [[nodiscard]] size_t aliveParticleCount() const;

    // ========================================================================
    // 更新与渲染
    // ========================================================================

    /**
     * @brief 更新所有粒子
     *
     * 更新粒子位置、生命周期等。
     *
     * @param world 客户端世界（用于碰撞检测和光照采样）
     */
    void tick(ClientWorld* world = nullptr);

    /**
     * @brief 渲染所有粒子
     *
     * @param cmd 命令缓冲区
     * @param projection 投影矩阵
     * @param view 视图矩阵
     * @param cameraPos 相机位置
     * @param frameIndex 当前帧索引
     */
    void render(VkCommandBuffer cmd,
                const glm::mat4& projection,
                const glm::mat4& view,
                const glm::vec3& cameraPos,
                u32 frameIndex);

    // ========================================================================
    // 纹理图集
    // ========================================================================

    /**
     * @brief 获取粒子纹理图集
     *
     * @return 纹理图集引用
     */
    [[nodiscard]] ParticleTextureAtlas& textureAtlas() { return m_textureAtlas; }
    [[nodiscard]] const ParticleTextureAtlas& textureAtlas() const { return m_textureAtlas; }

    // ========================================================================
    // 状态查询
    // ========================================================================

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    // ========================================================================
    // 资源创建
    // ========================================================================

    [[nodiscard]] Result<void> createVertexBuffer();
    [[nodiscard]] Result<void> createUniformBuffers();
    [[nodiscard]] Result<void> createDescriptorSetLayout();
    [[nodiscard]] Result<void> createDescriptorPool();
    [[nodiscard]] Result<void> createDescriptorSets();
    [[nodiscard]] Result<void> createPipelineLayout();
    [[nodiscard]] Result<void> createPipelines(VkSampleCountFlagBits sampleCount);
    [[nodiscard]] Result<void> createTexture();

    void updateUniformBuffer(u32 frameIndex);
    void updateVertexBuffer();

    // ========================================================================
    // Vulkan 辅助函数
    // ========================================================================

    [[nodiscard]] Result<u32> findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
    [[nodiscard]] Result<void> createBuffer(VkDeviceSize size,
                                            VkBufferUsageFlags usage,
                                            VkMemoryPropertyFlags properties,
                                            VkBuffer& buffer,
                                            VkDeviceMemory& memory);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer cmd);

private:
    // Vulkan 设备
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkExtent2D m_extent = {0, 0};
    bool m_initialized = false;

    // 粒子数据
    std::vector<std::unique_ptr<Particle>> m_particles;
    std::vector<ParticleVertex> m_vertexData;
    static constexpr size_t MAX_PARTICLES = 16384;  ///< 最大粒子数量

    // 纹理图集
    ParticleTextureAtlas m_textureAtlas;

    // 顶点缓冲区
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
    VkDeviceSize m_vertexBufferSize = 0;

    // 索引缓冲区（quad 索引可以复用）
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;

    // Uniform 缓冲区（每帧一个）
    static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
    std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_uniformBuffers = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> m_uniformBuffersMemory = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    std::array<void*, MAX_FRAMES_IN_FLIGHT> m_uniformBuffersMapped = {nullptr, nullptr};

    // 描述符
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_descriptorSets = {VK_NULL_HANDLE, VK_NULL_HANDLE};

    // 纹理
    VkImage m_textureImage = VK_NULL_HANDLE;
    VkDeviceMemory m_textureImageMemory = VK_NULL_HANDLE;
    VkImageView m_textureImageView = VK_NULL_HANDLE;
    VkSampler m_textureSampler = VK_NULL_HANDLE;

    // 管线
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    // 渲染状态
    glm::mat4 m_projection;
    glm::mat4 m_view;
    glm::vec3 m_cameraPos;
    f64 m_partialTick = 0.0f;
};

} // namespace mc::client::renderer::trident::particle
