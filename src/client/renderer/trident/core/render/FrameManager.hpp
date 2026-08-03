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
 * @brief 帧管理器
 *
 * 管理命令缓冲区、同步对象（信号量、栅栏）和帧生命周期。
 * 从 VulkanRenderer 拆分，职责单一化。
 */
class FrameManager {
public:
    FrameManager();
    ~FrameManager();

    // 禁止拷贝
    FrameManager(const FrameManager&) = delete;
    FrameManager& operator=(const FrameManager&) = delete;

    // 允许移动
    FrameManager(FrameManager&& other) noexcept;
    FrameManager& operator=(FrameManager&& other) noexcept;

    /**
     * @brief 初始化帧管理器
     * @param context Trident 上下文
     * @param maxFramesInFlight 最大同时在飞帧数
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(TridentContext* context, u32 maxFramesInFlight);

    /**
     * @brief 销毁所有资源
     */
    void destroy();

    /**
     * @brief 获取下一帧图像
     * @param swapchain 交换链
     * @return 图像索引或错误
     */
    [[nodiscard]] Result<u32> acquireNextImage(TridentSwapchain* swapchain);

    /**
     * @brief 提交并呈现帧
     * @param swapchain 交换链
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> submitAndPresent(TridentSwapchain* swapchain);

    /**
     * @brief 开始帧
     */
    void beginFrame();

    /**
     * @brief 结束帧
     */
    void endFrame();

    /**
     * @brief 等待帧完成
     */
    void waitForFrame(u32 frameIndex);

    // 访问器
    [[nodiscard]] VkCommandPool commandPool() const { return m_commandPool; }
    [[nodiscard]] VkCommandBuffer currentCommandBuffer() const;
    [[nodiscard]] u32 currentFrameIndex() const { return m_currentFrame; }
    [[nodiscard]] u32 currentImageIndex() const { return m_imageIndex; }
    [[nodiscard]] u32 maxFramesInFlight() const { return m_maxFramesInFlight; }
    [[nodiscard]] bool isFrameStarted() const { return m_frameStarted; }
    [[nodiscard]] bool isValid() const { return m_commandPool != VK_NULL_HANDLE; }

    /**
     * @brief 获取信号量（用于资源同步）
     */
    [[nodiscard]] VkSemaphore imageAvailableSemaphore(u32 frameIndex) const;
    [[nodiscard]] VkSemaphore renderFinishedSemaphore(u32 imageIndex) const;

private:
    // 创建方法
    [[nodiscard]] Result<void> _createCommandPool();
    [[nodiscard]] Result<void> _createCommandBuffers();
    [[nodiscard]] Result<void> _createSyncObjects();
    [[nodiscard]] Result<void> _ensureSwapchainResources(TridentSwapchain* swapchain);

    // 销毁方法
    void _destroyCommandPool();
    void _destroyCommandBuffers();
    void _destroySyncObjects();

    // 外部依赖
    TridentContext* m_context = nullptr;

    // 命令缓冲区
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_commandBuffers;

    // 同步对象
    std::vector<VkSemaphore> m_imageAvailableSemaphores; // 每帧一个
    std::vector<VkSemaphore> m_renderFinishedSemaphores; // 每个交换链图像一个
    std::vector<VkFence> m_inFlightFences;               // 每帧一个
    std::vector<VkFence> m_imageFences;                  // 每个交换链图像一个

    // 状态
    u32 m_maxFramesInFlight = 2;
    u32 m_currentFrame = 0;
    u32 m_imageIndex = 0;
    bool m_frameStarted = false;
    bool m_initialized = false;
};

} // namespace mc::client::renderer::trident
