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

#include "client/renderer/api/TridentApi.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident {

// 前置声明
class TridentContext;

/**
 * @brief 交换链配置
 */
struct SwapChainConfig {
    u32 width = 800;
    u32 height = 600;
    u32 imageCount = 3; // 三缓冲
    bool vsync = true;
    VkFormat preferredFormat = VK_FORMAT_B8G8R8A8_SRGB;
    VkColorSpaceKHR preferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
};

/**
 * @brief Trident 交换链
 *
 * 管理 Vulkan 交换链、图像和图像视图。
 * 从 VulkanSwapchain 重命名并移动到 trident 命名空间。
 */
class TridentSwapchain {
public:
    TridentSwapchain();
    ~TridentSwapchain();

    // 禁止拷贝
    TridentSwapchain(const TridentSwapchain&) = delete;
    TridentSwapchain& operator=(const TridentSwapchain&) = delete;

    // 允许移动
    TridentSwapchain(TridentSwapchain&& other) noexcept;
    TridentSwapchain& operator=(TridentSwapchain&& other) noexcept;

    /**
     * @brief 初始化交换链
     */
    [[nodiscard]] Result<void> initialize(TridentContext* context, const SwapChainConfig& config);

    /**
     * @brief 销毁所有资源
     */
    void destroy();

    /**
     * @brief 重建交换链（窗口大小变化时调用）
     */
    [[nodiscard]] Result<void> recreate(u32 width, u32 height);

    /**
     * @brief 设置 VSync 状态（在下次重建交换链时生效）
     */
    void setVSync(bool enabled) { m_config.vsync = enabled; }

    /**
     * @brief 获取下一帧图像
     */
    [[nodiscard]] Result<u32> acquireNextImage(VkSemaphore semaphore, VkFence fence = VK_NULL_HANDLE);

    /**
     * @brief 呈现图像
     */
    [[nodiscard]] Result<void> present(u32 imageIndex, VkSemaphore waitSemaphore);

    // 访问器
    [[nodiscard]] VkSwapchainKHR swapchain() const { return m_swapchain; }
    [[nodiscard]] VkFormat imageFormat() const { return m_imageFormat; }
    [[nodiscard]] VkFormat format() const { return m_imageFormat; } // 别名
    [[nodiscard]] VkColorSpaceKHR colorSpace() const { return m_colorSpace; }
    [[nodiscard]] VkExtent2D extent() const { return m_extent; }
    [[nodiscard]] const std::vector<VkImage>& images() const { return m_images; }
    [[nodiscard]] const std::vector<VkImageView>& imageViews() const { return m_imageViews; }
    [[nodiscard]] VkImageView imageView(u32 index) const;
    [[nodiscard]] u32 imageCount() const { return static_cast<u32>(m_images.size()); }
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    // 创建方法
    [[nodiscard]] Result<void> _createSwapchain();
    [[nodiscard]] Result<void> _createImageViews();
    void _destroySwapchain();
    void _destroyImageViews();

    // 辅助方法
    VkSurfaceFormatKHR _chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR _choosePresentMode(const std::vector<VkPresentModeKHR>& availableModes);
    VkExtent2D _chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    // 外部依赖
    TridentContext* m_context = nullptr;

    // Vulkan 对象
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_imageFormat = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR m_colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkExtent2D m_extent = {0, 0};

    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;

    // 配置
    SwapChainConfig m_config;
    bool m_initialized = false;
};

} // namespace mc::client::renderer::trident
