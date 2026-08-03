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

#include "Particle.hpp"
#include "ParticleTextureAtlas.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/settings/ClientSettings.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/frustum/Frustum.hpp"
#include "data/ParticleData.hpp"
#include <array>
#include <cstddef>
#include <memory>
#include <vector>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle {

/**
 * @brief 粒子管理器 Uniform 缓冲区数据结构
 */
struct ParticleUBO {
    alignas(16) glm::mat4 projection; ///< 投影矩阵
    alignas(16) glm::mat4 view;       ///< 视图矩阵
    alignas(16) glm::vec3 cameraPos;  ///< 相机位置
    alignas(4) f32 partialTick;       ///< 部分 tick
};

/**
 * @brief 待处理粒子数据
 *
 * 用于延迟生成粒子，避免在 tick 中途修改粒子列表。
 */
struct PendingParticle {
    ParticleTypeId type;                      ///< 粒子类型
    glm::vec3 position;                       ///< 位置
    glm::vec3 velocity;                       ///< 速度
    ClientWorld* world;                       ///< 世界指针（可为空）
    std::unique_ptr<data::ParticleData> data; ///< 粒子数据（可为空）
};

/**
 * @brief 粒子管理器
 *
 * 管理所有粒子的生命周期、更新和渲染。
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
    [[nodiscard]] Result<void> initialize(VkDevice device,
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
     * @brief 添加粒子到待处理队列
     *
     * 粒子将在下一帧开始时处理，避免在 tick 中途修改粒子列表。
     *
     * @param type 粒子类型
     * @param pos 位置
     * @param velocity 速度
     * @param world 世界指针（可为空）
     */
    void addPendingParticle(
        ParticleTypeId type, const glm::vec3& pos, const glm::vec3& velocity, ClientWorld* world = nullptr);

    /**
     * @brief 携带粒子数据添加到待处理队列
     *
     * 当粒子需要额外数据（如目标位置、颜色等）时使用此方法。
     * 粒子将在下一帧开始时处理，避免在 tick 中途修改粒子列表。
     *
     * @param type 粒子类型
     * @param pos 位置
     * @param velocity 速度
     * @param world 世界指针（可为空）
     * @param data 粒子数据（不可为空）
     */
    void addPendingParticle(ParticleTypeId type,
        const glm::vec3& pos,
        const glm::vec3& velocity,
        ClientWorld* world,
        std::unique_ptr<data::ParticleData> data);

    /**
     * @brief 设置相机位置（用于距离裁剪）
     *
     * @param pos 相机位置
     */
    void setCameraPosition(const glm::vec3& pos) { m_cameraPosition = pos; }

    /**
     * @brief 设置最大粒子距离
     *
     * @param distance 最大距离
     */
    void setMaxParticleDistance(f32 distance) { m_maxParticleDistance = distance; }

    /**
     * @brief 设置粒子效果模式
     *
     * 控制 ambient 粒子的过滤级别：
     * - Minimal: 仅显示 overrideLimiter=true 的重要粒子
     * - Decreased: 约 2/3 的普通粒子通过（每帧 1/3 概率降级为 Minimal）
     * - All: 显示所有粒子
     *
     * @param mode 粒子效果模式
     */
    void setParticleMode(client::ParticleMode mode) { m_particleMode = mode; }

    /**
     * @brief 获取当前粒子效果模式
     */
    [[nodiscard]] client::ParticleMode particleMode() const { return m_particleMode; }

    /**
     * @brief 判断是否应该生成/显示该类型的粒子
     *
     * 根据当前粒子效果模式和粒子类型的 overrideLimiter 标志，
     * 判断是否应该生成或显示该粒子。重要粒子（overrideLimiter=true）
     * 始终通过，不受模式影响。
     *
     * 在 Decreased 模式下，每帧对非重要粒子有 1/3 概率降级为 Minimal 行为。
     *
     * @param type 粒子类型 ID
     * @return true 如果粒子应该被生成/显示
     */
    [[nodiscard]] bool shouldShowParticle(ParticleTypeId type) const;

    /**
     * @brief 清除所有粒子
     */
    void clear();

    /**
     * @brief 清除所有待处理粒子
     *
     * 清除 pending 队列中的粒子。
     */
    void clearPending();

    /**
     * @brief 获取粒子数量
     */
    [[nodiscard]] size_t particleCount() const { return m_particles.size(); }

    /**
     * @brief 获取待处理粒子数量
     */
    [[nodiscard]] size_t pendingParticleCount() const { return m_pendingParticles.size(); }

    /**
     * @brief 获取存活的粒子数量
     */
    [[nodiscard]] size_t aliveParticleCount() const;

    // ========================================================================
    // 更新与渲染
    // ========================================================================

    /**
     * @brief 设置关联的客户端世界
     *
     * 某些粒子（如 VibrationSignalParticle 的实体来源变体）在 tick() 中需要
     * 通过 ClientWorld 重新解析实体位置。当 TridentEngine::render() 在无 world
     * 参数的情况下调用 tick() 时，ParticleManager 会使用此处设置的世界指针。
     *
     * @param world 客户端世界指针（可为空）
     */
    void setClientWorld(mc::client::ClientWorld* world) { m_clientWorld = world; }

    /**
     * @brief 获取关联的客户端世界
     */
    [[nodiscard]] mc::client::ClientWorld* clientWorld() const { return m_clientWorld; }

    /**
     * @brief 更新所有粒子
     *
     * 更新粒子位置、生命周期等。
     * 首先处理待处理粒子队列，然后更新所有粒子。
     * 对于发射器粒子，会处理其发射的新粒子。
     *
     * @param world 客户端世界（用于碰撞检测和光照采样）。为空时使用 setClientWorld() 设置的世界
     */
    void tick(mc::client::ClientWorld* world = nullptr);

    /**
     * @brief 处理待处理粒子队列
     *
     * 将 pending 队列中的粒子创建并添加到主粒子列表。
     */
    void processPendingParticles();

    // ========================================================================
    // 渲染
    // ========================================================================

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

    /**
     * @brief 渲染粒子（带视锥剔除）
     *
     * 只渲染在视锥内的粒子。使用球体测试进行剔除。
     *
     * @param cmd 命令缓冲区
     * @param projection 投影矩阵
     * @param view 视图矩阵
     * @param cameraPos 相机位置
     * @param frameIndex 当前帧索引
     * @param frustum 视锥体（用于剔除）
     */
    void render(VkCommandBuffer cmd,
        const glm::mat4& projection,
        const glm::mat4& view,
        const glm::vec3& cameraPos,
        u32 frameIndex,
        const mc::math::frustum::Frustum& frustum);

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

    [[nodiscard]] Result<void> _createVertexBuffer();
    [[nodiscard]] Result<void> _createUniformBuffers();
    [[nodiscard]] Result<void> _createDescriptorSetLayout();
    [[nodiscard]] Result<void> _createDescriptorPool();
    [[nodiscard]] Result<void> _createDescriptorSets();
    [[nodiscard]] Result<void> _createPipelineLayout();
    [[nodiscard]] Result<void> _createPipelines(VkSampleCountFlagBits sampleCount);
    [[nodiscard]] Result<void> _createTexture();

    void _updateUniformBuffer(u32 frameIndex);
    void _updateVertexBuffer();

    // ========================================================================
    // Vulkan 辅助函数
    // ========================================================================

    [[nodiscard]] Result<u32> _findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
    [[nodiscard]] Result<void> _createBuffer(VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& memory);
    VkCommandBuffer _beginSingleTimeCommands();
    void _endSingleTimeCommands(VkCommandBuffer cmd);

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
    std::vector<PendingParticle> m_pendingParticles; ///< 待处理粒子队列
    std::vector<ParticleVertex> m_vertexData;
    static constexpr size_t MAX_PARTICLES = 16384; ///< 最大粒子数量

    // 距离裁剪
    glm::vec3 m_cameraPosition = glm::vec3(0.0f); ///< 相机位置（用于距离裁剪）
    f32 m_maxParticleDistance = 256.0f;           ///< 最大粒子距离

    // 粒子效果模式
    client::ParticleMode m_particleMode = client::ParticleMode::All; ///< 粒子效果模式（默认全部显示）

    // 关联的客户端世界（用于 tick() 中需要世界参数的粒子，如实体来源的振动粒子）
    mc::client::ClientWorld* m_clientWorld = nullptr;

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
    f64 m_partialTick = 0.0;
};

} // namespace mc::client::renderer::trident::particle
