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

#include "TridentSwapchain.hpp"
#include "TridentContext.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::client::renderer::trident {

// ============================================================================
// 构造/析构
// ============================================================================

TridentSwapchain::TridentSwapchain() = default;

TridentSwapchain::~TridentSwapchain()
{
    destroy();
}

TridentSwapchain::TridentSwapchain(TridentSwapchain&& other) noexcept
    : m_context(other.m_context)
    , m_swapchain(other.m_swapchain)
    , m_imageFormat(other.m_imageFormat)
    , m_colorSpace(other.m_colorSpace)
    , m_extent(other.m_extent)
    , m_images(std::move(other.m_images))
    , m_imageViews(std::move(other.m_imageViews))
    , m_config(other.m_config)
    , m_initialized(other.m_initialized)
{
    other.m_context = nullptr;
    other.m_swapchain = VK_NULL_HANDLE;
    other.m_initialized = false;
}

TridentSwapchain& TridentSwapchain::operator=(TridentSwapchain&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_context = other.m_context;
        m_swapchain = other.m_swapchain;
        m_imageFormat = other.m_imageFormat;
        m_colorSpace = other.m_colorSpace;
        m_extent = other.m_extent;
        m_images = std::move(other.m_images);
        m_imageViews = std::move(other.m_imageViews);
        m_config = other.m_config;
        m_initialized = other.m_initialized;

        other.m_context = nullptr;
        other.m_swapchain = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> TridentSwapchain::initialize(TridentContext* context, const SwapChainConfig& config)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Initialization, "TridentSwapchain::initialize");

    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Swapchain already initialized");
    }

    if (!context) {
        return Error(ErrorCode::NullPointer, "Context is null");
    }

    m_context = context;
    m_config = config;

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Initialization, "TridentSwapchain::initialize::CreateSwapchain");
        auto result = _createSwapchain();
        if (result.failed()) {
            return result.error();
        }
    }

    m_initialized = true;
    spdlog::info("TridentSwapchain created: {}x{}, {} images, format: {}",
        m_extent.width,
        m_extent.height,
        m_images.size(),
        static_cast<int>(m_imageFormat));

    return {};
}

void TridentSwapchain::destroy()
{
    if (!m_initialized) return;

    _destroyImageViews();
    _destroySwapchain();

    m_context = nullptr;
    m_initialized = false;
    spdlog::info("TridentSwapchain destroyed");
}

Result<void> TridentSwapchain::recreate(u32 width, u32 height)
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "Swapchain not initialized");
    }

    m_config.width = width;
    m_config.height = height;

    m_context->waitIdle();
    _destroyImageViews();
    _destroySwapchain();

    auto result = _createSwapchain();
    if (result.failed()) {
        return result.error();
    }

    spdlog::info("TridentSwapchain recreated: {}x{}", m_extent.width, m_extent.height);
    return {};
}

Result<u32> TridentSwapchain::acquireNextImage(VkSemaphore semaphore, VkFence fence)
{
    u32 imageIndex;
    VkResult result =
        vkAcquireNextImageKHR(m_context->device(), m_swapchain, UINT64_MAX, semaphore, fence, &imageIndex);

    switch (result) {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            return imageIndex;
        case VK_ERROR_OUT_OF_DATE_KHR:
            return Error(ErrorCode::InvalidState, "Swapchain out of date");
        default:
            return Error(ErrorCode::Unknown, "Failed to acquire swapchain image: " + std::to_string(result));
    }
}

Result<void> TridentSwapchain::present(u32 imageIndex, VkSemaphore waitSemaphore)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &waitSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &imageIndex;

    VkResult result = vkQueuePresentKHR(m_context->presentQueue(), &presentInfo);

    switch (result) {
        case VK_SUCCESS:
            return {};
        case VK_SUBOPTIMAL_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
            return Error(ErrorCode::InvalidState, "Swapchain out of date");
        default:
            return Error(ErrorCode::Unknown, "Failed to present swapchain image: " + std::to_string(result));
    }
}

// ============================================================================
// 私有方法 - 创建
// ============================================================================

Result<void> TridentSwapchain::_createSwapchain()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Initialization, "TridentSwapchain::createSwapchain");

    SwapChainSupportDetails swapChainSupport = m_context->querySwapChainSupport();

    VkSurfaceFormatKHR surfaceFormat = _chooseSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = _choosePresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = _chooseExtent(swapChainSupport.capabilities);

    // 图像数量
    u32 imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // 确保不超过配置的数量
    imageCount = std::min(imageCount, m_config.imageCount);
    // 确保至少达到最小要求
    imageCount = std::max(imageCount, swapChainSupport.capabilities.minImageCount);

    // 交换链创建信息
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_context->surface();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // 支持blit操作

    // 队列族
    const auto& indices = m_context->queueFamilies();
    u32 queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Rendering.Initialization, "TridentSwapchain::createSwapchain::CreateSwapchainKHR");
        VkResult result = vkCreateSwapchainKHR(m_context->device(), &createInfo, nullptr, &m_swapchain);
        if (result != VK_SUCCESS) {
            return Error(ErrorCode::OperationFailed, "Failed to create swapchain: " + std::to_string(result));
        }
    }

    // 获取图像
    vkGetSwapchainImagesKHR(m_context->device(), m_swapchain, &imageCount, nullptr);
    m_images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_context->device(), m_swapchain, &imageCount, m_images.data());

    m_imageFormat = surfaceFormat.format;
    m_colorSpace = surfaceFormat.colorSpace;
    m_extent = extent;

    // 创建图像视图
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Rendering.Initialization, "TridentSwapchain::createSwapchain::CreateImageViews");
        auto viewResult = _createImageViews();
        if (viewResult.failed()) {
            _destroySwapchain();
            return viewResult.error();
        }
    }

    return {};
}

Result<void> TridentSwapchain::_createImageViews()
{
    m_imageViews.resize(m_images.size());

    for (size_t i = 0; i < m_images.size(); ++i) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_imageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(m_context->device(), &createInfo, nullptr, &m_imageViews[i]);
        if (result != VK_SUCCESS) {
            // 清理已创建的视图
            for (size_t j = 0; j < i; j++) {
                vkDestroyImageView(m_context->device(), m_imageViews[j], nullptr);
            }
            m_imageViews.clear();
            return Error(ErrorCode::OperationFailed, "Failed to create image view: " + std::to_string(result));
        }
    }

    return {};
}

void TridentSwapchain::_destroySwapchain()
{
    if (m_swapchain != VK_NULL_HANDLE && m_context) {
        vkDestroySwapchainKHR(m_context->device(), m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
    m_images.clear();
}

void TridentSwapchain::_destroyImageViews()
{
    if (!m_context) return;

    for (auto imageView : m_imageViews) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_context->device(), imageView, nullptr);
        }
    }
    m_imageViews.clear();
}

// ============================================================================
// 私有方法 - 辅助
// ============================================================================

VkSurfaceFormatKHR TridentSwapchain::_chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    // 首选格式
    for (const auto& format : availableFormats) {
        if (format.format == m_config.preferredFormat && format.colorSpace == m_config.preferredColorSpace) {
            return format;
        }
    }

    // 备选格式
    for (const auto& format : availableFormats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    // 使用第一个可用格式
    return availableFormats[0];
}

VkPresentModeKHR TridentSwapchain::_choosePresentMode(const std::vector<VkPresentModeKHR>& availableModes)
{
    // 首选模式
    VkPresentModeKHR preferredMode = m_config.vsync ? VK_PRESENT_MODE_FIFO_KHR : m_config.preferredPresentMode;

    // 检查是否支持首选模式
    for (const auto& mode : availableModes) {
        if (mode == preferredMode) {
            return mode;
        }
    }

    // Mailbox模式 (三缓冲)
    for (const auto& mode : availableModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }

    // Immediate模式 (无vsync)
    for (const auto& mode : availableModes) {
        if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            return mode;
        }
    }

    // FIFO保证可用
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D TridentSwapchain::_chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    VkExtent2D actualExtent = {
        std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, m_config.width)),
        std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, m_config.height))};

    return actualExtent;
}

// ============================================================================
// 访问器
// ============================================================================

VkImageView TridentSwapchain::imageView(u32 index) const
{
    if (index >= m_imageViews.size()) {
        return VK_NULL_HANDLE;
    }
    return m_imageViews[index];
}

} // namespace mc::client::renderer::trident
