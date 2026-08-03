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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client {

// 前向声明
class Camera;

} // namespace mc::client

namespace mc::client::renderer::trident::sky {

/**
 * @brief 天空 Uniform 缓冲区数据结构
 */
struct SkyUBO {
    alignas(16) glm::vec4 skyColor;         // 天空颜色
    alignas(16) glm::vec4 fogColor;         // 雾颜色
    alignas(16) glm::vec4 sunriseColor;     // 日出日落颜色 (A=强度)
    alignas(16) glm::vec4 sunriseDirection; // 日出日落中心方向 (xyz)
    alignas(16) glm::vec4 cameraForward;    // 摄像机前向 (xyz)
    alignas(4) f32 celestialAngle;          // 天体角度 (0.0-1.0)
    alignas(4) f32 starBrightness;          // 星星亮度
    alignas(4) i32 moonPhase;               // 月相 (0-7)
    alignas(4) f32 padding;
};

/**
 * @brief 天空渲染器
 *
 * 负责渲染天空、太阳、月亮和星星。
 */
class SkyRenderer {
public:
    SkyRenderer();
    ~SkyRenderer();

    // 禁止拷贝
    SkyRenderer(const SkyRenderer&) = delete;
    SkyRenderer& operator=(const SkyRenderer&) = delete;

    /**
     * @brief 初始化天空渲染器
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
     * @brief 销毁资源
     */
    void destroy();

    /**
     * @brief 窗口大小变化时重新创建资源
     * @param extent 新的交换链图像尺寸
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> onResize(VkExtent2D extent);

    /**
     * @brief 更新天空状态
     * @param dayTime 当前一天内的时间 (0-23999)
     * @param gameTime 游戏总 tick 数
     * @param partialTick 部分 tick (用于插值)
     * @param rainStrength 雨强度 (0.0-1.0)
     * @param thunderStrength 雷暴强度 (0.0-1.0)
     */
    void update(i64 dayTime, i64 gameTime, f64 partialTick, f64 rainStrength, f64 thunderStrength);

    /**
     * @brief 设置闪电闪烁亮度
     *
     * 当闪电击中时，天空会短暂变亮。
     *
     * @param brightness 闪电闪烁亮度 (0.0-1.0)，0表示无效果
     */
    void setLightningFlashBrightness(f64 brightness) { m_lightningFlashBrightness = brightness; }

    /**
     * @brief 渲染天空
     * @param cmd 命令缓冲区
     * @param projection 相机投影矩阵
     * @param view 相机视图矩阵
     * @param cameraPos 相机位置 (用于雾效果)
     * @param cameraForward 相机前向（用于下半天空晨昏填充）
     */
    void render(VkCommandBuffer cmd,
        const glm::mat4& projection,
        const glm::mat4& view,
        const glm::vec3& cameraPos,
        const glm::vec3& cameraForward,
        u32 frameIndex);

    // ========== 状态查询 ==========

    /**
     * @brief 获取当前天空颜色
     */
    [[nodiscard]] const glm::vec4& skyColor() const { return m_skyColor; }

    /**
     * @brief 获取当前雾颜色
     */
    [[nodiscard]] const glm::vec4& fogColor() const { return m_fogColor; }

    /**
     * @brief 获取太阳方向
     */
    [[nodiscard]] const glm::vec3& sunDirection() const { return m_sunDirection; }

    /**
     * @brief 获取太阳强度
     */
    [[nodiscard]] f64 sunIntensity() const { return m_sunIntensity; }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    // ========== 资源创建 ==========

    /**
     * @brief 创建天空穹顶 VBO
     */
    [[nodiscard]] Result<void> _createSkyDomeVBO();

    /**
     * @brief 创建星星 VBO
     *
     * 星星位置使用固定种子 (10842L) 生成，保证一致性
     * 约 1500 颗星星分布在单位球面上
     */
    [[nodiscard]] Result<void> _createStarVBO();

    /**
     * @brief 创建太阳/月亮 VBO
     */
    [[nodiscard]] Result<void> _createSunMoonVBO();

    /**
     * @brief 创建 Uniform 缓冲区
     */
    [[nodiscard]] Result<void> _createUniformBuffers();

    /**
     * @brief 创建描述符集布局
     */
    [[nodiscard]] Result<void> _createDescriptorSetLayout();

    /**
     * @brief 创建描述符池和描述符集
     */
    [[nodiscard]] Result<void> _createDescriptorSets();

    /**
     * @brief 创建管线布局
     */
    [[nodiscard]] Result<void> _createPipelineLayout();

    /**
     * @brief 创建图形管线
     */
    [[nodiscard]] Result<void> _createPipelines(VkSampleCountFlagBits sampleCount);

    /**
     * @brief 更新 Uniform 缓冲区
     * @param frameIndex 当前帧索引
     */
    void _updateUniformBuffer(u32 frameIndex);

    // ========== Vulkan 辅助函数 ==========

    /**
     * @brief 查找内存类型
     */
    [[nodiscard]] Result<u32> _findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);

    /**
     * @brief 创建缓冲区
     */
    [[nodiscard]] Result<void> _createBuffer(VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& memory);

    /**
     * @brief 开始单次命令
     */
    VkCommandBuffer _beginSingleTimeCommands();

    /**
     * @brief 结束单次命令
     */
    void _endSingleTimeCommands(VkCommandBuffer commandBuffer);

    // ========== 渲染方法 ==========

    /**
     * @brief 渲染天空穹顶
     */
    void _renderSkyDome(VkCommandBuffer cmd);

    /**
     * @brief 渲染太阳
     */
    void _renderSun(VkCommandBuffer cmd);

    /**
     * @brief 渲染月亮
     */
    void _renderMoon(VkCommandBuffer cmd);

    /**
     * @brief 渲染星星
     */
    void _renderStars(VkCommandBuffer cmd);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkExtent2D m_extent = {0, 0};
    bool m_initialized = false;

    // Vulkan 资源
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

    // 管线
    VkPipeline m_skyPipeline = VK_NULL_HANDLE;  // 天空穹顶管线
    VkPipeline m_sunPipeline = VK_NULL_HANDLE;  // 太阳管线
    VkPipeline m_moonPipeline = VK_NULL_HANDLE; // 月亮管线
    VkPipeline m_starPipeline = VK_NULL_HANDLE; // 星星管线

    // 顶点缓冲区
    VkBuffer m_skyDomeVBO = VK_NULL_HANDLE;
    VkDeviceMemory m_skyDomeVBOMemory = VK_NULL_HANDLE;
    VkBuffer m_skyDomeIBO = VK_NULL_HANDLE;
    VkDeviceMemory m_skyDomeIBOMemory = VK_NULL_HANDLE;
    VkBuffer m_starVBO = VK_NULL_HANDLE;
    VkDeviceMemory m_starVBOMemory = VK_NULL_HANDLE;
    VkBuffer m_sunMoonVBO = VK_NULL_HANDLE;
    VkDeviceMemory m_sunMoonVBOMemory = VK_NULL_HANDLE;

    u32 m_skyDomeIndexCount = 0;
    u32 m_starVertexCount = 0;

    // Uniform 缓冲区 (每帧一个)
    static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
    VkBuffer m_uniformBuffers[MAX_FRAMES_IN_FLIGHT] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkDeviceMemory m_uniformBuffersMemory[MAX_FRAMES_IN_FLIGHT] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    void* m_uniformBuffersMapped[MAX_FRAMES_IN_FLIGHT] = {nullptr, nullptr};
    VkDescriptorSet m_descriptorSets[MAX_FRAMES_IN_FLIGHT] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    u32 m_currentFrame = 0;

    // 天空状态
    i64 m_dayTime = 0;
    i64 m_gameTime = 0;
    f64 m_celestialAngle = 0.0;
    i32 m_moonPhase = 0;
    f64 m_starBrightness = 0.0;
    glm::vec4 m_skyColor = glm::vec4(120.0f / 255.0f, 167.0f / 255.0f, 1.0f, 1.0f);
    glm::vec4 m_fogColor = glm::vec4(0.7f, 0.75f, 0.8f, 1.0f);
    glm::vec4 m_sunriseSunsetColor = glm::vec4(0.0f);
    glm::vec3 m_sunriseDirection = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 m_cameraForward = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_sunDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    f64 m_sunIntensity = 1.0;
    f64 m_rainStrength = 0.0;
    f64 m_thunderStrength = 0.0;
    f64 m_lightningFlashBrightness = 0.0; ///< 闪电闪烁亮度 (0.0-1.0)
};

} // namespace mc::client::renderer::trident::sky

// 向后兼容别名
namespace mc::client {
using SkyRenderer = renderer::trident::sky::SkyRenderer;
}
