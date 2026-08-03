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
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer {

/**
 * @brief Vulkan 上下文句柄
 *
 * 包含常用 Vulkan 句柄的结构体，用于传递给需要直接 Vulkan 访问的类。
 * 避免每个类都存储单独的句柄成员变量。
 */
struct VulkanContextHandles {
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;

    [[nodiscard]] bool isValid() const
    {
        return device != VK_NULL_HANDLE && physicalDevice != VK_NULL_HANDLE && commandPool != VK_NULL_HANDLE &&
            graphicsQueue != VK_NULL_HANDLE;
    }
};

/**
 * @brief Vulkan 工具函数集合
 *
 * 提供常用的 Vulkan 操作工具函数，避免代码重复。
 * 这些函数设计为独立使用，不依赖于特定的类。
 */
namespace VulkanUtils {

/**
 * @brief 查找合适的内存类型索引
 *
 * @param physicalDevice 物理设备
 * @param typeFilter 内存类型过滤器
 * @param properties 所需内存属性
 * @return 内存类型索引或错误
 */
[[nodiscard]] inline Result<u32> findMemoryType(
    VkPhysicalDevice physicalDevice, u32 typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    return Error(ErrorCode::NotFound, "Failed to find suitable memory type");
}

/**
 * @brief 创建 Vulkan 缓冲区
 *
 * @param device 逻辑设备
 * @param physicalDevice 物理设备
 * @param size 缓冲区大小
 * @param usage 用途标志
 * @param properties 内存属性
 * @param outBuffer [out] 创建的缓冲区
 * @param outMemory [out] 分配的内存
 * @return 成功或错误
 */
[[nodiscard]] inline Result<void> createBuffer(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& outBuffer,
    VkDeviceMemory& outMemory)
{
    // 创建缓冲区
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &outBuffer);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create buffer");
    }

    // 获取内存需求
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, outBuffer, &memRequirements);

    // 查找内存类型
    auto memTypeResult = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);
    if (!memTypeResult.success()) {
        vkDestroyBuffer(device, outBuffer, nullptr);
        outBuffer = VK_NULL_HANDLE;
        return memTypeResult.error();
    }

    // 分配内存
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memTypeResult.value();

    result = vkAllocateMemory(device, &allocInfo, nullptr, &outMemory);
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(device, outBuffer, nullptr);
        outBuffer = VK_NULL_HANDLE;
        return Error(ErrorCode::OutOfMemory, "Failed to allocate buffer memory");
    }

    // 绑定内存
    result = vkBindBufferMemory(device, outBuffer, outMemory, 0);
    if (result != VK_SUCCESS) {
        vkFreeMemory(device, outMemory, nullptr);
        outMemory = VK_NULL_HANDLE;
        vkDestroyBuffer(device, outBuffer, nullptr);
        outBuffer = VK_NULL_HANDLE;
        return Error(ErrorCode::OperationFailed, "Failed to bind buffer memory");
    }

    return Result<void>::ok();
}

/**
 * @brief 创建 Vulkan 图像
 *
 * @param device 逻辑设备
 * @param physicalDevice 物理设备
 * @param width 图像宽度
 * @param height 图像高度
 * @param format 图像格式
 * @param tiling 图像平铺模式
 * @param usage 用途标志
 * @param properties 内存属性
 * @param outImage [out] 创建的图像
 * @param outMemory [out] 分配的内存
 * @return 成功或错误
 */
[[nodiscard]] inline Result<void> createImage(VkDevice device,
    VkPhysicalDevice physicalDevice,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage& outImage,
    VkDeviceMemory& outMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    VkResult result = vkCreateImage(device, &imageInfo, nullptr, &outImage);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create image");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, outImage, &memRequirements);

    auto memTypeResult = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);
    if (!memTypeResult.success()) {
        vkDestroyImage(device, outImage, nullptr);
        outImage = VK_NULL_HANDLE;
        return memTypeResult.error();
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memTypeResult.value();

    result = vkAllocateMemory(device, &allocInfo, nullptr, &outMemory);
    if (result != VK_SUCCESS) {
        vkDestroyImage(device, outImage, nullptr);
        outImage = VK_NULL_HANDLE;
        return Error(ErrorCode::OutOfMemory, "Failed to allocate image memory");
    }

    result = vkBindImageMemory(device, outImage, outMemory, 0);
    if (result != VK_SUCCESS) {
        vkFreeMemory(device, outMemory, nullptr);
        outMemory = VK_NULL_HANDLE;
        vkDestroyImage(device, outImage, nullptr);
        outImage = VK_NULL_HANDLE;
        return Error(ErrorCode::OperationFailed, "Failed to bind image memory");
    }

    return Result<void>::ok();
}

/**
 * @brief 创建图像视图
 *
 * @param device 逻辑设备
 * @param image 图像
 * @param format 格式
 * @param aspectMask 方面掩码
 * @param outView [out] 创建的图像视图
 * @return 成功或错误
 */
[[nodiscard]] inline Result<void> createImageView(
    VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectMask, VkImageView& outView)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &outView);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to create image view");
    }

    return Result<void>::ok();
}

/**
 * @brief 开始单次命令
 *
 * @param device 逻辑设备
 * @param commandPool 命令池
 * @return 命令缓冲区
 */
[[nodiscard]] inline VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

/**
 * @brief 结束单次命令（使用 fence，非阻塞）
 *
 * 此版本使用 fence 来等待命令完成，而不是阻塞整个队列。
 * 性能优于 vkQueueWaitIdle 版本。
 *
 * @param device 逻辑设备
 * @param commandPool 命令池
 * @param graphicsQueue 图形队列
 * @param commandBuffer 命令缓冲区
 * @param fence 可选的 fence（如果提供则使用，否则创建临时 fence）
 * @note 如果 fence 为 VK_NULL_HANDLE，会创建临时 fence 并等待
 */
inline void endSingleTimeCommands(VkDevice device,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    VkCommandBuffer commandBuffer,
    VkFence fence = VK_NULL_HANDLE)
{
    vkEndCommandBuffer(commandBuffer);

    bool useTempFence = (fence == VK_NULL_HANDLE);
    VkFence tempFence = VK_NULL_HANDLE;

    if (useTempFence) {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vkCreateFence(device, &fenceInfo, nullptr, &tempFence);
        fence = tempFence;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    if (useTempFence) {
        vkDestroyFence(device, tempFence, nullptr);
    }

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

/**
 * @brief 结束单次命令并提交（不等待）
 *
 * 使用提供的 fence 提交命令，但不等待完成。
 * 调用者负责等待 fence 并清理资源。
 *
 * @param device 逻辑设备
 * @param commandPool 命令池
 * @param graphicsQueue 图形队列
 * @param commandBuffer 命令缓冲区
 * @param fence 用于同步的 fence
 */
inline void endSingleTimeCommandsAsync(
    VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkCommandBuffer commandBuffer, VkFence fence)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence);
    // 命令缓冲区在等待 fence 后由调用者释放
}

/**
 * @brief 转换图像布局
 *
 * @param commandBuffer 命令缓冲区
 * @param image 图像
 * @param oldLayout 旧布局
 * @param newLayout 新布局
 * @param srcStage 源管线阶段
 * @param dstStage 目标管线阶段
 * @param aspectMask 方面掩码（默认为颜色）
 */
inline void transitionImageLayout(VkCommandBuffer commandBuffer,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage,
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // 设置访问掩码
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    } else {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;
    }

    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

/**
 * @brief 复制缓冲区到图像
 *
 * @param commandBuffer 命令缓冲区
 * @param srcBuffer 源缓冲区
 * @param dstImage 目标图像
 * @param width 图像宽度
 * @param height 图像高度
 */
inline void copyBufferToImage(
    VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, u32 width, u32 height)
{
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

/**
 * @brief 创建纹理采样器
 *
 * @param device 逻辑设备
 * @param magFilter 放大过滤
 * @param minFilter 缩小过滤
 * @param addressModeU U 方向寻址模式
 * @param addressModeV V 方向寻址模式
 * @param outSampler [out] 创建的采样器
 * @return 成功或错误
 */
[[nodiscard]] inline Result<void> createSampler(VkDevice device,
    VkFilter magFilter,
    VkFilter minFilter,
    VkSamplerAddressMode addressModeU,
    VkSamplerAddressMode addressModeV,
    VkSampler& outSampler)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = magFilter;
    samplerInfo.minFilter = minFilter;
    samplerInfo.addressModeU = addressModeU;
    samplerInfo.addressModeV = addressModeV;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &outSampler);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OperationFailed, "Failed to create sampler");
    }

    return Result<void>::ok();
}

/**
 * @brief 销毁缓冲区
 *
 * @param device 逻辑设备
 * @param buffer 缓冲区句柄（会被置为 VK_NULL_HANDLE）
 * @param memory 内存句柄（会被置为 VK_NULL_HANDLE）
 */
inline void destroyBuffer(VkDevice device, VkBuffer& buffer, VkDeviceMemory& memory)
{
    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
    }
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, memory, nullptr);
        memory = VK_NULL_HANDLE;
    }
}

/**
 * @brief 销毁图像
 *
 * @param device 逻辑设备
 * @param image 图像句柄（会被置为 VK_NULL_HANDLE）
 * @param memory 内存句柄（会被置为 VK_NULL_HANDLE）
 * @param view 图像视图句柄（会被置为 VK_NULL_HANDLE，可选）
 */
inline void destroyImage(VkDevice device, VkImage& image, VkDeviceMemory& memory, VkImageView& view)
{
    if (view != VK_NULL_HANDLE) {
        vkDestroyImageView(device, view, nullptr);
        view = VK_NULL_HANDLE;
    }
    if (image != VK_NULL_HANDLE) {
        vkDestroyImage(device, image, nullptr);
        image = VK_NULL_HANDLE;
    }
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, memory, nullptr);
        memory = VK_NULL_HANDLE;
    }
}

} // namespace VulkanUtils
} // namespace mc::client::renderer
