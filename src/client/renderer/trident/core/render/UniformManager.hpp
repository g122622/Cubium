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
#include <vector>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident {

// 前置声明
class TridentContext;
class DescriptorManager;

/**
 * @brief 相机 Uniform 缓冲区数据
 */
struct CameraUBO {
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewProjection;
};

/**
 * @brief 光照 Uniform 缓冲区数据
 */
struct LightingUBO {
    glm::vec3 sunDirection;
    f32 sunIntensity;
    glm::vec3 moonDirection;
    f32 moonIntensity;
    f32 dayTime;
    f32 gameTime;
    f32 _padding[2];
};

/**
 * @brief 雾模式枚举
 */
enum class FogMode : i32 {
    None = 0,   // 禁用雾效果
    Linear = 1, // 线性雾（陆地）
    Exp2 = 2    // 指数雾（水中/岩浆）
};

/**
 * @brief 雾效果 Uniform 缓冲区数据
 *
 * 线性雾用于陆地场景，指数雾用于水下和岩浆环境。
 */
struct FogUBO {
    alignas(4) f32 fogStart;        // 雾开始距离（线性雾）
    alignas(4) f32 fogEnd;          // 雾结束距离（线性雾）
    alignas(4) f32 fogDensity;      // 指数雾密度（水中/岩浆）
    alignas(4) i32 fogMode;         // 雾模式 (FogMode 枚举值)
    alignas(16) glm::vec4 fogColor; // 雾颜色（RGBA）
};

/**
 * @brief Uniform 缓冲区管理器
 *
 * 管理 Uniform 缓冲区的创建、更新和绑定。
 * 从 VulkanRenderer 拆分，职责单一化。
 */
class UniformManager {
public:
    UniformManager();
    ~UniformManager();

    // 禁止拷贝
    UniformManager(const UniformManager&) = delete;
    UniformManager& operator=(const UniformManager&) = delete;

    // 允许移动
    UniformManager(UniformManager&& other) noexcept;
    UniformManager& operator=(UniformManager&& other) noexcept;

    /**
     * @brief 初始化 Uniform 管理器
     * @param context Trident 上下文
     * @param descriptor 描述符管理器
     * @param maxFramesInFlight 最大同时在飞帧数
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(
        TridentContext* context, DescriptorManager* descriptor, u32 maxFramesInFlight);

    /**
     * @brief 销毁所有资源
     */
    void destroy();

    /**
     * @brief 更新相机 Uniform
     * @param viewMatrix 视图矩阵
     * @param projectionMatrix 投影矩阵
     * @param frameIndex 帧索引
     */
    void updateCamera(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, u32 frameIndex);

    /**
     * @brief 更新光照 Uniform
     * @param dayTime 一天内的时间 (0-23999)
     * @param gameTime 游戏总 tick 数
     * @param partialTick 部分 tick
     */
    void updateLighting(i64 dayTime, i64 gameTime, f64 partialTick);

    // 访问器
    [[nodiscard]] VkBuffer cameraBuffer(u32 frameIndex) const;
    [[nodiscard]] VkBuffer lightingBuffer() const { return m_lightingBuffer; }
    [[nodiscard]] VkDescriptorSet cameraDescriptorSet(u32 frameIndex) const;
    [[nodiscard]] bool isValid() const { return !m_cameraBuffers.empty() && m_cameraBuffers[0] != VK_NULL_HANDLE; }

private:
    // 创建方法
    [[nodiscard]] Result<void> _createUniformBuffers();
    [[nodiscard]] Result<void> _createCameraDescriptorSets();
    void _destroyUniformBuffers();

    // 外部依赖
    TridentContext* m_context = nullptr;
    DescriptorManager* m_descriptor = nullptr;

    // Uniform 缓冲区
    std::vector<VkBuffer> m_cameraBuffers;
    std::vector<VkDeviceMemory> m_cameraBufferMemory;
    std::vector<void*> m_cameraBufferMapped;

    VkBuffer m_lightingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_lightingBufferMemory = VK_NULL_HANDLE;
    void* m_lightingBufferMapped = nullptr;

    // 描述符集
    std::vector<VkDescriptorSet> m_cameraDescriptorSets;

    // 配置
    u32 m_maxFramesInFlight = 0;
    bool m_initialized = false;
};

} // namespace mc::client::renderer::trident
