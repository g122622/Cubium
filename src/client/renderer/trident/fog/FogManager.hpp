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

#include "client/renderer/trident/core/render/UniformManager.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <vector>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident::fog {

/**
 * @brief 雾效果管理器
 *
 * 负责计算和管理雾效果参数。
 *
 * 雾效果参数：
 * - 线性雾（陆地）：fogStart 和 fogEnd 控制雾的范围
 *   - fogStart = farPlaneDistance * 0.75
 *   - fogEnd = farPlaneDistance
 *
 * - 指数雾（水中/岩浆）：fogDensity 控制密度
 *   - 水中：density = 0.05
 *   - 岩浆：density = 0.25
 *
 * 雾颜色与天空颜色、天气、时间相关联。
 */
class FogManager {
public:
    FogManager();
    ~FogManager();

    // 禁止拷贝
    FogManager(const FogManager&) = delete;
    FogManager& operator=(const FogManager&) = delete;

    // 允许移动
    FogManager(FogManager&& other) noexcept;
    FogManager& operator=(FogManager&& other) noexcept;

    /**
     * @brief 初始化雾管理器
     *
     * @param device Vulkan 逻辑设备
     * @param physicalDevice Vulkan 物理设备
     * @param descriptorPool 描述符池
     * @param layout 描述符集布局（雾 UBO 布局）
     * @param maxFramesInFlight 最大同时在飞帧数
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDescriptorPool descriptorPool,
        VkDescriptorSetLayout layout,
        u32 maxFramesInFlight = 2);

    /**
     * @brief 销毁所有资源
     */
    void destroy();

    /**
     * @brief 更新雾参数
     *
     * 根据渲染距离、天气和天空颜色计算雾参数。
     *
     * @param renderDistanceChunks 渲染距离（区块数）
     * @param rainStrength 雨强度 (0.0-1.0)
     * @param thunderStrength 雷暴强度 (0.0-1.0)
     * @param skyFogColor 天空雾颜色（来自 SkyRenderer）
     * @param cameraPos 相机位置（用于高度雾）
     * @param cameraSkyLight 相机所在方块的天空光照等级 (0-15)，影响雨天雾距缩短程度
     * @param cameraBiomeHasPrecip 相机所在生物群系是否降水，降水群系雨天雾更浓
     */
    void update(i32 renderDistanceChunks,
        f64 rainStrength,
        f64 thunderStrength,
        f64 landFogDensity,
        const glm::vec4& skyFogColor,
        const glm::vec3& cameraPos,
        f32 cameraSkyLight,
        bool cameraBiomeHasPrecip);

    /**
     * @brief 设置雾模式
     *
     * @param mode 雾模式
     */
    void setFogMode(FogMode mode);

    /**
     * @brief 设置水中雾参数
     *
     * 切换到指数雾模式并设置水的雾密度。
     * @param waterFogColor 水下雾颜色 (RGB格式，如 0x050533)
     */
    void setUnderwater(u32 waterFogColor);

    /**
     * @brief 设置岩浆雾参数
     *
     * 切换到指数雾模式并设置岩浆的雾密度。
     */
    void setInLava();

    /**
     * @brief 重置为陆地雾
     *
     * 切换回线性雾模式。
     */
    void resetToLand();

    /**
     * @brief 获取指定帧的描述符集
     *
     * @param frameIndex 帧索引
     * @return 描述符集
     */
    [[nodiscard]] VkDescriptorSet descriptorSet(u32 frameIndex) const;

    /**
     * @brief 获取当前雾 UBO 数据
     */
    [[nodiscard]] const FogUBO& fogUBO() const { return m_fogUBO; }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    /**
     * @brief 计算线性雾参数
     *
     * @param renderDistance 渲染距离（方块数）
     */
    void _calculateLinearFog(f64 renderDistance);

    /**
     * @brief 更新 Uniform 缓冲区
     *
     * @param frameIndex 帧索引
     */
    void _updateUniformBuffer(u32 frameIndex);

    /**
     * @brief 创建 Uniform 缓冲区
     */
    [[nodiscard]] Result<void> _createUniformBuffers();

    /**
     * @brief 创建描述符集
     */
    [[nodiscard]] Result<void> _createDescriptorSets();

    /**
     * @brief 销毁 Uniform 缓冲区
     */
    void _destroyUniformBuffers();

    /**
     * @brief 查找合适的内存类型
     */
    [[nodiscard]] Result<u32> _findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;

    // Uniform 缓冲区（每帧一个）
    std::vector<VkBuffer> m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformMemory;
    std::vector<void*> m_mappedMemory;

    // 描述符集（每帧一个）
    std::vector<VkDescriptorSet> m_descriptorSets;

    // 雾参数
    FogUBO m_fogUBO{};

    // 配置
    u32 m_maxFramesInFlight = 2;
    bool m_initialized = false;

    // 当前雾模式
    FogMode m_currentFogMode = FogMode::Linear;

    // 雨天雾距缩短倍率（平滑插值状态，对齐原版 AtmosphericFogEnvironment.rainFogMultiplier）
    f32 m_rainFogMultiplier = 0.0f;
};

} // namespace mc::client::renderer::trident::fog
