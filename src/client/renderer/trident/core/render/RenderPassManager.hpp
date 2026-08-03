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
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident {

// 前置声明
class TridentContext;
class TridentSwapchain;

/**
 * @brief 渲染通道和帧缓冲区管理器
 *
 * 负责 Vulkan 渲染通道、深度缓冲区和帧缓冲区的创建与管理。
 * 从 VulkanRenderer 拆分，职责单一化。
 */
class RenderPassManager {
public:
    RenderPassManager();
    ~RenderPassManager();

    // 禁止拷贝
    RenderPassManager(const RenderPassManager&) = delete;
    RenderPassManager& operator=(const RenderPassManager&) = delete;

    // 允许移动
    RenderPassManager(RenderPassManager&& other) noexcept;
    RenderPassManager& operator=(RenderPassManager&& other) noexcept;

    /**
     * @brief 初始化渲染通道管理器
     * @param context Trident 上下文
     * @param swapchain 交换链
     * @param sampleCount 主渲染通道使用的多重采样等级
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(
        TridentContext* context, TridentSwapchain* swapchain, VkSampleCountFlagBits sampleCount);

    /**
     * @brief 销毁所有资源
     */
    void destroy();

    /**
     * @brief 重建帧缓冲区（窗口大小变化时调用）
     * @param width 新宽度
     * @param height 新高度
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> recreate(u32 width, u32 height);

    // 访问器
    [[nodiscard]] VkRenderPass renderPass() const { return m_renderPass; }
    [[nodiscard]] VkFramebuffer framebuffer(u32 index) const;
    [[nodiscard]] VkImageView depthImageView() const { return m_depthImageView; }
    [[nodiscard]] VkFormat depthFormat() const { return m_depthFormat; }
    [[nodiscard]] VkSampleCountFlagBits sampleCount() const { return m_sampleCount; }
    [[nodiscard]] bool isValid() const { return m_renderPass != VK_NULL_HANDLE; }
    [[nodiscard]] u32 framebufferCount() const { return static_cast<u32>(m_framebuffers.size()); }

private:
    // 创建方法
    [[nodiscard]] Result<void> _createRenderPass();
    [[nodiscard]] Result<void> _createColorResources();
    [[nodiscard]] Result<void> _createDepthResources();
    [[nodiscard]] Result<void> _createFramebuffers();

    // 销毁方法
    void _destroyRenderPass();
    void _destroyColorResources();
    void _destroyDepthResources();
    void _destroyFramebuffers();

    // 外部依赖
    TridentContext* m_context = nullptr;
    TridentSwapchain* m_swapchain = nullptr;

    // Vulkan 对象
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_framebuffers;

    VkImage m_colorImage = VK_NULL_HANDLE;
    VkDeviceMemory m_colorImageMemory = VK_NULL_HANDLE;
    VkImageView m_colorImageView = VK_NULL_HANDLE;

    // 深度缓冲区
    VkImage m_depthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
    VkImageView m_depthImageView = VK_NULL_HANDLE;
    VkFormat m_depthFormat = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits m_sampleCount = VK_SAMPLE_COUNT_1_BIT;

    // 状态
    bool m_initialized = false;
};

} // namespace mc::client::renderer::trident
