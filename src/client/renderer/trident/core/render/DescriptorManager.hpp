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
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident {

// 前置声明
class TridentContext;

/**
 * @brief 描述符管理器
 *
 * 管理描述符集布局、描述符池和描述符集分配。
 * 从 VulkanRenderer 拆分，职责单一化。
 */
class DescriptorManager {
public:
    DescriptorManager();
    ~DescriptorManager();

    // 禁止拷贝
    DescriptorManager(const DescriptorManager&) = delete;
    DescriptorManager& operator=(const DescriptorManager&) = delete;

    // 允许移动
    DescriptorManager(DescriptorManager&& other) noexcept;
    DescriptorManager& operator=(DescriptorManager&& other) noexcept;

    /**
     * @brief 初始化描述符管理器
     * @param context Trident 上下文
     * @param maxFramesInFlight 最大同时在飞帧数
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(TridentContext* context, u32 maxFramesInFlight = 2);

    /**
     * @brief 销毁所有资源
     */
    void destroy();

    /**
     * @brief 分配相机描述符集
     * @param frameIndex 帧索引
     * @return 描述符集或错误
     */
    [[nodiscard]] Result<VkDescriptorSet> allocateCameraSet(u32 frameIndex);

    /**
     * @brief 分配纹理描述符集
     * @return 描述符集或错误
     */
    [[nodiscard]] Result<VkDescriptorSet> allocateTextureSet();

    /**
     * @brief 分配雾效果描述符集
     * @return 描述符集或错误
     */
    [[nodiscard]] Result<VkDescriptorSet> allocateFogSet();

    // 访问器
    [[nodiscard]] VkDescriptorSetLayout cameraLayout() const { return m_cameraLayout; }
    [[nodiscard]] VkDescriptorSetLayout textureLayout() const { return m_textureLayout; }
    [[nodiscard]] VkDescriptorSetLayout fogLayout() const { return m_fogLayout; }
    [[nodiscard]] VkDescriptorPool pool() const { return m_pool; }
    [[nodiscard]] VkPipelineLayout pipelineLayout() const { return m_pipelineLayout; }
    [[nodiscard]] bool isValid() const { return m_pool != VK_NULL_HANDLE; }

private:
    // 创建方法
    [[nodiscard]] Result<void> _createDescriptorSetLayouts();
    [[nodiscard]] Result<void> _createPipelineLayout();
    [[nodiscard]] Result<void> _createDescriptorPool();

    // 销毁方法
    void _destroyDescriptorSetLayouts();
    void _destroyPipelineLayout();
    void _destroyDescriptorPool();

    // 分配辅助方法
    [[nodiscard]] Result<VkDescriptorSet> _allocateSet(VkDescriptorSetLayout layout, const char* errorMsg);

    // 外部依赖
    TridentContext* m_context = nullptr;

    // 描述符集布局
    VkDescriptorSetLayout m_cameraLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_textureLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_fogLayout = VK_NULL_HANDLE;

    // 管线布局
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

    // 描述符池
    VkDescriptorPool m_pool = VK_NULL_HANDLE;

    // 配置
    u32 m_maxFramesInFlight = 2;
    bool m_initialized = false;
};

} // namespace mc::client::renderer::trident
