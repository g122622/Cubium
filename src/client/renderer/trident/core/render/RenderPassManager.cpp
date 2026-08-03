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

#include "RenderPassManager.hpp"
#include "client/renderer/trident/core/TridentContext.hpp"
#include "client/renderer/trident/core/TridentSwapchain.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <array>
#include <cstddef>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace {

[[nodiscard]] VkImageAspectFlags depthAspectMask(VkFormat format)
{
    switch (format) {
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        default:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
    }
}

[[nodiscard]] mc::Result<void> createAttachmentImage(mc::client::renderer::trident::TridentContext* context,
    VkExtent2D extent,
    VkFormat format,
    VkImageUsageFlags usage,
    VkSampleCountFlagBits sampleCount,
    VkImageAspectFlags aspectMask,
    VkImage& outImage,
    VkDeviceMemory& outMemory,
    VkImageView& outImageView)
{
    VkDevice device = context->device();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = sampleCount;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateImage(device, &imageInfo, nullptr, &outImage);
    if (result != VK_SUCCESS) {
        return mc::Error(
            mc::ErrorCode::OperationFailed, "Failed to create attachment image: " + std::to_string(result));
    }

    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(device, outImage, &memRequirements);

    auto memoryTypeResult =
        context->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memoryTypeResult.failed()) {
        vkDestroyImage(device, outImage, nullptr);
        outImage = VK_NULL_HANDLE;
        return memoryTypeResult.error();
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeResult.value();

    result = vkAllocateMemory(device, &allocInfo, nullptr, &outMemory);
    if (result != VK_SUCCESS) {
        vkDestroyImage(device, outImage, nullptr);
        outImage = VK_NULL_HANDLE;
        return mc::Error(
            mc::ErrorCode::OutOfMemory, "Failed to allocate attachment image memory: " + std::to_string(result));
    }

    result = vkBindImageMemory(device, outImage, outMemory, 0);
    if (result != VK_SUCCESS) {
        vkFreeMemory(device, outMemory, nullptr);
        outMemory = VK_NULL_HANDLE;
        vkDestroyImage(device, outImage, nullptr);
        outImage = VK_NULL_HANDLE;
        return mc::Error(
            mc::ErrorCode::OperationFailed, "Failed to bind attachment image memory: " + std::to_string(result));
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = outImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    result = vkCreateImageView(device, &viewInfo, nullptr, &outImageView);
    if (result != VK_SUCCESS) {
        vkFreeMemory(device, outMemory, nullptr);
        outMemory = VK_NULL_HANDLE;
        vkDestroyImage(device, outImage, nullptr);
        outImage = VK_NULL_HANDLE;
        return mc::Error(
            mc::ErrorCode::OperationFailed, "Failed to create attachment image view: " + std::to_string(result));
    }

    return mc::Result<void>::ok();
}

} // namespace

namespace mc::client::renderer::trident {

// ============================================================================
// 构造/析构
// ============================================================================

RenderPassManager::RenderPassManager() = default;

RenderPassManager::~RenderPassManager()
{
    destroy();
}

RenderPassManager::RenderPassManager(RenderPassManager&& other) noexcept
    : m_context(other.m_context)
    , m_swapchain(other.m_swapchain)
    , m_renderPass(other.m_renderPass)
    , m_framebuffers(std::move(other.m_framebuffers))
    , m_colorImage(other.m_colorImage)
    , m_colorImageMemory(other.m_colorImageMemory)
    , m_colorImageView(other.m_colorImageView)
    , m_depthImage(other.m_depthImage)
    , m_depthImageMemory(other.m_depthImageMemory)
    , m_depthImageView(other.m_depthImageView)
    , m_depthFormat(other.m_depthFormat)
    , m_sampleCount(other.m_sampleCount)
    , m_initialized(other.m_initialized)
{
    other.m_context = nullptr;
    other.m_swapchain = nullptr;
    other.m_renderPass = VK_NULL_HANDLE;
    other.m_colorImage = VK_NULL_HANDLE;
    other.m_colorImageMemory = VK_NULL_HANDLE;
    other.m_colorImageView = VK_NULL_HANDLE;
    other.m_depthImage = VK_NULL_HANDLE;
    other.m_depthImageMemory = VK_NULL_HANDLE;
    other.m_depthImageView = VK_NULL_HANDLE;
    other.m_sampleCount = VK_SAMPLE_COUNT_1_BIT;
    other.m_initialized = false;
}

RenderPassManager& RenderPassManager::operator=(RenderPassManager&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_context = other.m_context;
        m_swapchain = other.m_swapchain;
        m_renderPass = other.m_renderPass;
        m_framebuffers = std::move(other.m_framebuffers);
        m_colorImage = other.m_colorImage;
        m_colorImageMemory = other.m_colorImageMemory;
        m_colorImageView = other.m_colorImageView;
        m_depthImage = other.m_depthImage;
        m_depthImageMemory = other.m_depthImageMemory;
        m_depthImageView = other.m_depthImageView;
        m_depthFormat = other.m_depthFormat;
        m_sampleCount = other.m_sampleCount;
        m_initialized = other.m_initialized;

        other.m_context = nullptr;
        other.m_swapchain = nullptr;
        other.m_renderPass = VK_NULL_HANDLE;
        other.m_colorImage = VK_NULL_HANDLE;
        other.m_colorImageMemory = VK_NULL_HANDLE;
        other.m_colorImageView = VK_NULL_HANDLE;
        other.m_depthImage = VK_NULL_HANDLE;
        other.m_depthImageMemory = VK_NULL_HANDLE;
        other.m_depthImageView = VK_NULL_HANDLE;
        other.m_sampleCount = VK_SAMPLE_COUNT_1_BIT;
        other.m_initialized = false;
    }
    return *this;
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> RenderPassManager::initialize(
    TridentContext* context, TridentSwapchain* swapchain, VkSampleCountFlagBits sampleCount)
{
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "RenderPassManager already initialized");
    }

    if (!context || !swapchain) {
        return Error(ErrorCode::NullPointer, "Context or swapchain is null");
    }

    m_context = context;
    m_swapchain = swapchain;
    m_sampleCount = sampleCount;

    // 创建渲染通道
    auto renderPassResult = _createRenderPass();
    if (renderPassResult.failed()) {
        return renderPassResult.error();
    }

    // 创建深度缓冲区
    auto depthResult = _createDepthResources();
    if (depthResult.failed()) {
        _destroyRenderPass();
        return depthResult.error();
    }

    auto colorResult = _createColorResources();
    if (colorResult.failed()) {
        _destroyDepthResources();
        _destroyRenderPass();
        return colorResult.error();
    }

    // 创建帧缓冲区
    auto framebufferResult = _createFramebuffers();
    if (framebufferResult.failed()) {
        _destroyColorResources();
        _destroyDepthResources();
        _destroyRenderPass();
        return framebufferResult.error();
    }

    m_initialized = true;
    spdlog::info("RenderPassManager initialized successfully");
    return {};
}

void RenderPassManager::destroy()
{
    if (!m_initialized) return;

    _destroyFramebuffers();
    _destroyColorResources();
    _destroyDepthResources();
    _destroyRenderPass();

    m_context = nullptr;
    m_swapchain = nullptr;
    m_sampleCount = VK_SAMPLE_COUNT_1_BIT;
    m_initialized = false;

    spdlog::info("RenderPassManager destroyed");
}

Result<void> RenderPassManager::recreate(u32 width, u32 height)
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "RenderPassManager not initialized");
    }

    m_context->waitIdle();

    _destroyFramebuffers();
    _destroyColorResources();
    _destroyDepthResources();

    auto depthResult = _createDepthResources();
    if (depthResult.failed()) {
        return depthResult.error();
    }

    auto colorResult = _createColorResources();
    if (colorResult.failed()) {
        _destroyDepthResources();
        return colorResult.error();
    }

    auto framebufferResult = _createFramebuffers();
    if (framebufferResult.failed()) {
        _destroyColorResources();
        return framebufferResult.error();
    }

    return {};
}

VkFramebuffer RenderPassManager::framebuffer(u32 index) const
{
    if (index >= m_framebuffers.size()) {
        return VK_NULL_HANDLE;
    }
    return m_framebuffers[index];
}

// ============================================================================
// 私有方法 - 创建
// ============================================================================

Result<void> RenderPassManager::_createRenderPass()
{
    // 查找深度格式
    auto depthFormatResult = m_context->findDepthFormat();
    if (depthFormatResult.failed()) {
        return depthFormatResult.error();
    }
    m_depthFormat = depthFormatResult.value();

    VkAttachmentDescription colorAttachment{};
    VkAttachmentDescription depthAttachment{};
    VkAttachmentDescription resolveAttachment{};
    VkAttachmentReference colorAttachmentRef{};
    VkAttachmentReference depthAttachmentRef{};
    VkAttachmentReference resolveAttachmentRef{};
    VkSubpassDescription subpass{};
    VkSubpassDependency dependency{};
    std::array<VkAttachmentDescription, 3> attachments{};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    if (m_sampleCount == VK_SAMPLE_COUNT_1_BIT) {
        colorAttachment.format = m_swapchain->imageFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        depthAttachment.format = m_depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        attachments[0] = colorAttachment;
        attachments[1] = depthAttachment;
        renderPassInfo.attachmentCount = static_cast<u32>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
    } else {
        colorAttachment.format = m_swapchain->imageFormat();
        colorAttachment.samples = m_sampleCount;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        depthAttachment.format = m_depthFormat;
        depthAttachment.samples = m_sampleCount;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        resolveAttachment.format = m_swapchain->imageFormat();
        resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        resolveAttachmentRef.attachment = 2;
        resolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pResolveAttachments = &resolveAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        attachments[0] = colorAttachment;
        attachments[1] = depthAttachment;
        attachments[2] = resolveAttachment;
        renderPassInfo.attachmentCount = 3;
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
    }

    VkResult result = vkCreateRenderPass(m_context->device(), &renderPassInfo, nullptr, &m_renderPass);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to create render pass: " + std::to_string(result));
    }

    return {};
}

Result<void> RenderPassManager::_createColorResources()
{
    if (m_sampleCount == VK_SAMPLE_COUNT_1_BIT) {
        return mc::Result<void>::ok();
    }

    return createAttachmentImage(m_context,
        m_swapchain->extent(),
        m_swapchain->imageFormat(),
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        m_sampleCount,
        VK_IMAGE_ASPECT_COLOR_BIT,
        m_colorImage,
        m_colorImageMemory,
        m_colorImageView);
}

Result<void> RenderPassManager::_createDepthResources()
{
    return createAttachmentImage(m_context,
        m_swapchain->extent(),
        m_depthFormat,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        m_sampleCount,
        depthAspectMask(m_depthFormat),
        m_depthImage,
        m_depthImageMemory,
        m_depthImageView);
}

Result<void> RenderPassManager::_createFramebuffers()
{
    VkExtent2D extent = m_swapchain->extent();
    const auto& imageViews = m_swapchain->imageViews();

    m_framebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); i++) {
        if (m_sampleCount != VK_SAMPLE_COUNT_1_BIT && m_colorImageView == VK_NULL_HANDLE) {
            return Error(ErrorCode::InvalidState, "MSAA color image view is null");
        }

        if (m_sampleCount == VK_SAMPLE_COUNT_1_BIT) {
            std::array<VkImageView, 2> attachments = {imageViews[i], m_depthImageView};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass;
            framebufferInfo.attachmentCount = static_cast<u32>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1;

            VkResult result = vkCreateFramebuffer(m_context->device(), &framebufferInfo, nullptr, &m_framebuffers[i]);

            if (result != VK_SUCCESS) {
                // 清理已创建的帧缓冲区
                for (size_t j = 0; j < i; j++) {
                    vkDestroyFramebuffer(m_context->device(), m_framebuffers[j], nullptr);
                }
                m_framebuffers.clear();
                return Error(ErrorCode::OperationFailed, "Failed to create framebuffer: " + std::to_string(result));
            }

            continue;
        }

        std::array<VkImageView, 3> attachments = {m_colorImageView, m_depthImageView, imageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<u32>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(m_context->device(), &framebufferInfo, nullptr, &m_framebuffers[i]);

        if (result != VK_SUCCESS) {
            // 清理已创建的帧缓冲区
            for (size_t j = 0; j < i; j++) {
                vkDestroyFramebuffer(m_context->device(), m_framebuffers[j], nullptr);
            }
            m_framebuffers.clear();
            return Error(ErrorCode::OperationFailed, "Failed to create framebuffer: " + std::to_string(result));
        }
    }

    return {};
}

// ============================================================================
// 私有方法 - 销毁
// ============================================================================

void RenderPassManager::_destroyRenderPass()
{
    if (m_renderPass != VK_NULL_HANDLE && m_context) {
        vkDestroyRenderPass(m_context->device(), m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }
}

void RenderPassManager::_destroyColorResources()
{
    VkDevice device = m_context ? m_context->device() : VK_NULL_HANDLE;

    if (m_colorImageView != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_colorImageView, nullptr);
        m_colorImageView = VK_NULL_HANDLE;
    }

    if (m_colorImage != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_colorImage, nullptr);
        m_colorImage = VK_NULL_HANDLE;
    }

    if (m_colorImageMemory != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_colorImageMemory, nullptr);
        m_colorImageMemory = VK_NULL_HANDLE;
    }
}

void RenderPassManager::_destroyDepthResources()
{
    VkDevice device = m_context ? m_context->device() : VK_NULL_HANDLE;

    if (m_depthImageView != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_depthImageView, nullptr);
        m_depthImageView = VK_NULL_HANDLE;
    }

    if (m_depthImage != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_depthImage, nullptr);
        m_depthImage = VK_NULL_HANDLE;
    }

    if (m_depthImageMemory != VK_NULL_HANDLE && device != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_depthImageMemory, nullptr);
        m_depthImageMemory = VK_NULL_HANDLE;
    }
}

void RenderPassManager::_destroyFramebuffers()
{
    if (!m_context) return;

    VkDevice device = m_context->device();
    for (auto framebuffer : m_framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    m_framebuffers.clear();
}

} // namespace mc::client::renderer::trident
