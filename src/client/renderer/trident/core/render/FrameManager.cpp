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

#include "FrameManager.hpp"
#include "client/renderer/trident/core/TridentContext.hpp"
#include "client/renderer/trident/core/TridentSwapchain.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <cstdint>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident {

// ============================================================================
// 构造/析构
// ============================================================================

FrameManager::FrameManager() = default;

FrameManager::~FrameManager()
{
    destroy();
}

FrameManager::FrameManager(FrameManager&& other) noexcept
    : m_context(other.m_context)
    , m_commandPool(other.m_commandPool)
    , m_commandBuffers(std::move(other.m_commandBuffers))
    , m_imageAvailableSemaphores(std::move(other.m_imageAvailableSemaphores))
    , m_renderFinishedSemaphores(std::move(other.m_renderFinishedSemaphores))
    , m_inFlightFences(std::move(other.m_inFlightFences))
    , m_imageFences(std::move(other.m_imageFences))
    , m_maxFramesInFlight(other.m_maxFramesInFlight)
    , m_currentFrame(other.m_currentFrame)
    , m_imageIndex(other.m_imageIndex)
    , m_frameStarted(other.m_frameStarted)
    , m_initialized(other.m_initialized)
{
    other.m_context = nullptr;
    other.m_commandPool = VK_NULL_HANDLE;
    other.m_initialized = false;
}

FrameManager& FrameManager::operator=(FrameManager&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_context = other.m_context;
        m_commandPool = other.m_commandPool;
        m_commandBuffers = std::move(other.m_commandBuffers);
        m_imageAvailableSemaphores = std::move(other.m_imageAvailableSemaphores);
        m_renderFinishedSemaphores = std::move(other.m_renderFinishedSemaphores);
        m_inFlightFences = std::move(other.m_inFlightFences);
        m_imageFences = std::move(other.m_imageFences);
        m_maxFramesInFlight = other.m_maxFramesInFlight;
        m_currentFrame = other.m_currentFrame;
        m_imageIndex = other.m_imageIndex;
        m_frameStarted = other.m_frameStarted;
        m_initialized = other.m_initialized;

        other.m_context = nullptr;
        other.m_commandPool = VK_NULL_HANDLE;
        other.m_initialized = false;
    }
    return *this;
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> FrameManager::initialize(TridentContext* context, u32 maxFramesInFlight)
{
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "FrameManager already initialized");
    }

    if (!context) {
        return Error(ErrorCode::NullPointer, "Context is null");
    }

    m_context = context;
    m_maxFramesInFlight = maxFramesInFlight;

    // 创建命令池
    auto poolResult = _createCommandPool();
    if (poolResult.failed()) {
        return poolResult.error();
    }

    // 创建同步对象
    auto syncResult = _createSyncObjects();
    if (syncResult.failed()) {
        _destroyCommandPool();
        return syncResult.error();
    }

    m_initialized = true;
    spdlog::info("FrameManager initialized with {} frames in flight", m_maxFramesInFlight);
    return {};
}

void FrameManager::destroy()
{
    if (!m_initialized) return;

    m_context->waitIdle();

    _destroySyncObjects();
    _destroyCommandBuffers();
    _destroyCommandPool();

    m_context = nullptr;
    m_initialized = false;
    spdlog::info("FrameManager destroyed");
}

// ============================================================================
// 帧管理
// ============================================================================

Result<u32> FrameManager::acquireNextImage(TridentSwapchain* swapchain)
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "FrameManager not initialized");
    }

    if (!swapchain) {
        return Error(ErrorCode::NullPointer, "Swapchain is null");
    }

    auto ensureResult = _ensureSwapchainResources(swapchain);
    if (ensureResult.failed()) {
        return ensureResult.error();
    }

    // 等待当前帧完成
    vkWaitForFences(m_context->device(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    // 获取下一帧图像
    VkResult result = vkAcquireNextImageKHR(m_context->device(),
        swapchain->swapchain(),
        UINT64_MAX,
        m_imageAvailableSemaphores[m_currentFrame],
        VK_NULL_HANDLE,
        &m_imageIndex);

    switch (result) {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            // 检查该图像是否正在使用
            if (m_imageFences[m_imageIndex] != VK_NULL_HANDLE) {
                vkWaitForFences(m_context->device(), 1, &m_imageFences[m_imageIndex], VK_TRUE, UINT64_MAX);
            }
            m_imageFences[m_imageIndex] = m_inFlightFences[m_currentFrame];
            return m_imageIndex;
        case VK_ERROR_OUT_OF_DATE_KHR:
            return Error(ErrorCode::InvalidState, "Swapchain out of date");
        default: {
            return Error(ErrorCode::Unknown, "Failed to acquire swapchain image: " + std::to_string(result));
        }
    }
}

Result<void> FrameManager::submitAndPresent(TridentSwapchain* swapchain)
{
    if (!m_frameStarted) {
        return Error(ErrorCode::InvalidState, "Frame not started");
    }

    if (!swapchain) {
        return Error(ErrorCode::NullPointer, "Swapchain is null");
    }

    MC_ASSERT_RELEASE(m_imageIndex < m_commandBuffers.size());
    MC_ASSERT_RELEASE(m_imageIndex < m_renderFinishedSemaphores.size());

    // 提交命令缓冲区
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_imageIndex];

    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_imageIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // 重置栅栏
    vkResetFences(m_context->device(), 1, &m_inFlightFences[m_currentFrame]);

    VkResult result = vkQueueSubmit(m_context->graphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]);

    if (result != VK_SUCCESS) {
        // 提交失败（典型为 VK_ERROR_DEVICE_LOST=-4）。设备已不可用，后续帧不可能成功。
        // 仍须推进帧簿记并清 m_frameStarted：否则下一帧 beginFrame 因 m_frameStarted=true
        // 早退、acquireNextImage 在刚 reset 但永不会 signaled 的 fence 上无限等待。
        // 真正的设备丢失恢复/关闭由上层（TridentEngine/ClientApplication）处理。
        m_currentFrame = (m_currentFrame + 1) % m_maxFramesInFlight;
        m_frameStarted = false;
        return Error(ErrorCode::OperationFailed, "Failed to submit draw command buffer: " + std::to_string(result));
    }

    // 呈现
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    VkSwapchainKHR swapchainHandle = swapchain->swapchain();
    presentInfo.pSwapchains = &swapchainHandle;
    presentInfo.pImageIndices = &m_imageIndex;

    result = vkQueuePresentKHR(m_context->presentQueue(), &presentInfo);

    switch (result) {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            return Error(ErrorCode::InvalidState, "Swapchain out of date");
        default:
            return Error(ErrorCode::Unknown, "Failed to present swapchain image: " + std::to_string(result));
    }

    // 推进帧索引
    m_currentFrame = (m_currentFrame + 1) % m_maxFramesInFlight;
    m_frameStarted = false;

    return {};
}

void FrameManager::beginFrame()
{
    if (m_frameStarted) return;

    MC_ASSERT_RELEASE(m_imageIndex < m_commandBuffers.size());

    // 重置命令缓冲区
    vkResetCommandBuffer(m_commandBuffers[m_imageIndex], 0);

    // 开始命令缓冲区录制
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(m_commandBuffers[m_imageIndex], &beginInfo);
    m_frameStarted = true;
}

void FrameManager::endFrame()
{
    if (!m_frameStarted) return;

    MC_ASSERT_RELEASE(m_imageIndex < m_commandBuffers.size());

    vkEndCommandBuffer(m_commandBuffers[m_imageIndex]);
    // 注意：不在这里设置 m_frameStarted = false，因为 submitAndPresent 会处理
}

void FrameManager::waitForFrame(u32 frameIndex)
{
    MC_ASSERT_RELEASE(frameIndex < m_maxFramesInFlight);

    vkWaitForFences(m_context->device(), 1, &m_inFlightFences[frameIndex], VK_TRUE, UINT64_MAX);
}

VkCommandBuffer FrameManager::currentCommandBuffer() const
{
    MC_ASSERT_RELEASE(m_imageIndex < m_commandBuffers.size());
    return m_commandBuffers[m_imageIndex];
}

VkSemaphore FrameManager::imageAvailableSemaphore(u32 frameIndex) const
{
    MC_ASSERT_RELEASE(frameIndex < m_imageAvailableSemaphores.size());
    return m_imageAvailableSemaphores[frameIndex];
}

VkSemaphore FrameManager::renderFinishedSemaphore(u32 imageIndex) const
{
    MC_ASSERT_RELEASE(imageIndex < m_renderFinishedSemaphores.size());
    return m_renderFinishedSemaphores[imageIndex];
}

// ============================================================================
// 私有方法 - 创建
// ============================================================================

Result<void> FrameManager::_createCommandPool()
{
    const auto& queueFamilies = m_context->queueFamilies();

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilies.graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult result = vkCreateCommandPool(m_context->device(), &poolInfo, nullptr, &m_commandPool);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to create command pool: " + std::to_string(result));
    }

    return {};
}

Result<void> FrameManager::_createCommandBuffers()
{
    // 需要在交换链创建后调用，这里预留接口
    // 实际命令缓冲区数量取决于交换链图像数量
    return {};
}

Result<void> FrameManager::_createSyncObjects()
{
    m_imageAvailableSemaphores.resize(m_maxFramesInFlight);
    m_inFlightFences.resize(m_maxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkDevice device = m_context->device();

    for (u32 i = 0; i < m_maxFramesInFlight; i++) {
        VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]);
        if (result != VK_SUCCESS) {
            _destroySyncObjects();
            return Error(ErrorCode::OperationFailed, "Failed to create semaphore: " + std::to_string(result));
        }

        result = vkCreateFence(device, &fenceInfo, nullptr, &m_inFlightFences[i]);
        if (result != VK_SUCCESS) {
            _destroySyncObjects();
            return Error(ErrorCode::OperationFailed, "Failed to create fence: " + std::to_string(result));
        }
    }

    // renderFinishedSemaphores 和 imageFences 在交换链创建后初始化
    return {};
}

Result<void> FrameManager::_ensureSwapchainResources(TridentSwapchain* swapchain)
{
    if (!swapchain) {
        return Error(ErrorCode::NullPointer, "Swapchain is null");
    }

    const u32 imageCount = swapchain->imageCount();
    if (imageCount == 0) {
        return Error(ErrorCode::InvalidState, "Swapchain has no images");
    }

    VkDevice device = m_context->device();

    // 命令缓冲区（每个交换链图像一个）
    if (m_commandBuffers.size() != imageCount) {
        if (!m_commandBuffers.empty()) {
            vkFreeCommandBuffers(
                device, m_commandPool, static_cast<u32>(m_commandBuffers.size()), m_commandBuffers.data());
            m_commandBuffers.clear();
        }

        m_commandBuffers.resize(imageCount, VK_NULL_HANDLE);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = imageCount;

        VkResult allocResult = vkAllocateCommandBuffers(device, &allocInfo, m_commandBuffers.data());
        if (allocResult != VK_SUCCESS) {
            m_commandBuffers.clear();
            return Error(
                ErrorCode::OperationFailed, "Failed to allocate command buffers: " + std::to_string(allocResult));
        }
    }

    // 每个交换链图像一个 render-finished 信号量
    if (m_renderFinishedSemaphores.size() != imageCount) {
        for (auto& semaphore : m_renderFinishedSemaphores) {
            if (semaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(device, semaphore, nullptr);
            }
        }
        m_renderFinishedSemaphores.clear();
        m_renderFinishedSemaphores.resize(imageCount, VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (u32 i = 0; i < imageCount; ++i) {
            VkResult semResult = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
            if (semResult != VK_SUCCESS) {
                for (u32 j = 0; j <= i; ++j) {
                    if (m_renderFinishedSemaphores[j] != VK_NULL_HANDLE) {
                        vkDestroySemaphore(device, m_renderFinishedSemaphores[j], nullptr);
                        m_renderFinishedSemaphores[j] = VK_NULL_HANDLE;
                    }
                }
                m_renderFinishedSemaphores.clear();
                return Error(ErrorCode::OperationFailed,
                    "Failed to create render-finished semaphore: " + std::to_string(semResult));
            }
        }
    }

    // 每个交换链图像一个 in-flight fence 句柄记录
    if (m_imageFences.size() != imageCount) {
        m_imageFences.assign(imageCount, VK_NULL_HANDLE);
    }

    if (m_imageIndex >= imageCount) {
        m_imageIndex = 0;
    }

    return {};
}

// ============================================================================
// 私有方法 - 销毁
// ============================================================================

void FrameManager::_destroyCommandPool()
{
    if (m_commandPool != VK_NULL_HANDLE && m_context) {
        vkDestroyCommandPool(m_context->device(), m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }
}

void FrameManager::_destroyCommandBuffers()
{
    // 命令缓冲区随命令池销毁而销毁
    m_commandBuffers.clear();
}

void FrameManager::_destroySyncObjects()
{
    if (!m_context) return;

    VkDevice device = m_context->device();

    for (auto& semaphore : m_imageAvailableSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
    }
    m_imageAvailableSemaphores.clear();

    for (auto& semaphore : m_renderFinishedSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
    }
    m_renderFinishedSemaphores.clear();

    for (auto& fence : m_inFlightFences) {
        if (fence != VK_NULL_HANDLE) {
            vkDestroyFence(device, fence, nullptr);
        }
    }
    m_inFlightFences.clear();

    m_imageFences.clear();
}

} // namespace mc::client::renderer::trident
