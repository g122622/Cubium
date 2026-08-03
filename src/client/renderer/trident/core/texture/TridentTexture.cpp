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

#include "TridentTexture.hpp"
#include "client/renderer/api/texture/ITexture.hpp"
#include "client/renderer/api/texture/ITextureAtlas.hpp"
#include "client/renderer/api/texture/TextureRegion.hpp"
#include "client/renderer/trident/core/TridentContext.hpp"
#include "client/renderer/trident/core/buffer/TridentBuffer.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <cstring>
#include <utility>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident {

// ============================================================================
// TridentTexture 实现
// ============================================================================

TridentTexture::TridentTexture() = default;

TridentTexture::~TridentTexture()
{
    destroy();
}

TridentTexture::TridentTexture(TridentTexture&& other) noexcept
    : m_context(other.m_context)
    , m_image(other.m_image)
    , m_memory(other.m_memory)
    , m_imageView(other.m_imageView)
    , m_sampler(other.m_sampler)
    , m_vkFormat(other.m_vkFormat)
    , m_format(other.m_format)
    , m_layout(other.m_layout)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_depth(other.m_depth)
    , m_mipLevels(other.m_mipLevels)
    , m_arrayLayers(other.m_arrayLayers)
    , m_ownsImage(other.m_ownsImage)
{
    other.m_context = nullptr;
    other.m_image = VK_NULL_HANDLE;
    other.m_memory = VK_NULL_HANDLE;
    other.m_imageView = VK_NULL_HANDLE;
    other.m_sampler = VK_NULL_HANDLE;
    other.m_vkFormat = VK_FORMAT_UNDEFINED;
}

TridentTexture& TridentTexture::operator=(TridentTexture&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_context = other.m_context;
        m_image = other.m_image;
        m_memory = other.m_memory;
        m_imageView = other.m_imageView;
        m_sampler = other.m_sampler;
        m_vkFormat = other.m_vkFormat;
        m_format = other.m_format;
        m_layout = other.m_layout;
        m_width = other.m_width;
        m_height = other.m_height;
        m_depth = other.m_depth;
        m_mipLevels = other.m_mipLevels;
        m_arrayLayers = other.m_arrayLayers;
        m_ownsImage = other.m_ownsImage;

        other.m_context = nullptr;
        other.m_image = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_imageView = VK_NULL_HANDLE;
        other.m_sampler = VK_NULL_HANDLE;
        other.m_vkFormat = VK_FORMAT_UNDEFINED;
    }
    return *this;
}

Result<void> TridentTexture::create(TridentContext* context, const api::TextureDesc& desc)
{
    if (!context) {
        return Error(ErrorCode::NullPointer, "Context is null");
    }

    if (m_image != VK_NULL_HANDLE) {
        destroy();
    }

    m_context = context;
    m_width = desc.width;
    m_height = desc.height;
    m_depth = desc.depth;
    m_mipLevels = desc.mipLevels;
    m_arrayLayers = desc.arrayLayers;
    m_format = desc.format;
    m_vkFormat = toVkFormat(desc.format);
    m_ownsImage = true;

    // 确定图像用途
    VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (desc.generateMipmaps && desc.mipLevels > 1) {
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    // 创建图像
    auto result = createImage(usage, VK_IMAGE_TILING_OPTIMAL);
    if (result.failed()) {
        return result;
    }

    // 创建图像视图
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    if (desc.format == api::TextureFormat::D16_UNORM || desc.format == api::TextureFormat::D24_UNORM_S8_UINT ||
        desc.format == api::TextureFormat::D32_FLOAT) {
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    result = createImageView(aspect);
    if (result.failed()) {
        destroy();
        return result;
    }

    // 创建采样器
    result = createSampler(toVkFilter(desc.magFilter), toVkFilter(desc.minFilter), toVkAddressMode(desc.addressModeU));
    if (result.failed()) {
        destroy();
        return result;
    }

    return {};
}

Result<void> TridentTexture::createFromExisting(
    TridentContext* context, VkImage image, VkImageView imageView, VkFormat format, u32 width, u32 height)
{
    if (!context) {
        return Error(ErrorCode::NullPointer, "Context is null");
    }

    if (image == VK_NULL_HANDLE || imageView == VK_NULL_HANDLE) {
        return Error(ErrorCode::InvalidArgument, "Invalid image or image view");
    }

    m_context = context;
    m_image = image;
    m_imageView = imageView;
    m_vkFormat = format;
    m_width = width;
    m_height = height;
    m_depth = 1;
    m_mipLevels = 1;
    m_arrayLayers = 1;
    m_ownsImage = false;
    m_layout = VK_IMAGE_LAYOUT_UNDEFINED;

    // 根据格式设置纹理格式
    switch (format) {
        case VK_FORMAT_R8G8B8A8_UNORM:
            m_format = api::TextureFormat::R8G8B8A8_UNORM;
            break;
        case VK_FORMAT_R8G8B8A8_SRGB:
            m_format = api::TextureFormat::R8G8B8A8_SRGB;
            break;
        default:
            m_format = api::TextureFormat::R8G8B8A8_SRGB;
            break;
    }

    return {};
}

void TridentTexture::destroy()
{
    if (m_context == nullptr) return;

    VkDevice device = m_context->device();
    if (device == VK_NULL_HANDLE) return;

    // 只有当拥有图像资源时才销毁
    if (m_ownsImage) {
        if (m_sampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, m_sampler, nullptr);
            m_sampler = VK_NULL_HANDLE;
        }

        if (m_imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, m_imageView, nullptr);
            m_imageView = VK_NULL_HANDLE;
        }

        if (m_image != VK_NULL_HANDLE) {
            vkDestroyImage(device, m_image, nullptr);
            m_image = VK_NULL_HANDLE;
        }

        if (m_memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, m_memory, nullptr);
            m_memory = VK_NULL_HANDLE;
        }
    }

    m_context = nullptr;
    m_width = 0;
    m_height = 0;
    m_depth = 1;
    m_mipLevels = 1;
    m_arrayLayers = 1;
    m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    m_vkFormat = VK_FORMAT_UNDEFINED;
    m_ownsImage = true;
}

Result<void> TridentTexture::upload(const void* data, u64 size, u32 level)
{
    if (m_image == VK_NULL_HANDLE || !m_context) {
        return Error(ErrorCode::OperationFailed, "Texture not initialized");
    }

    if (!data || size == 0) {
        return Error(ErrorCode::InvalidArgument, "Invalid data pointer or size");
    }

    // 通过统一暂存缓冲池上传：stage → memcpy → 单次命令录制 buffer→image → release
    auto* pool = m_context->stagingPool();
    auto handle = pool->stage(size);
    MC_ASSERT_RELEASE_MSG(handle.valid, "TridentTexture::upload: staging pool out of space");
    std::memcpy(handle.mappedPtr, data, static_cast<size_t>(size));

    VkCommandBuffer cmd = m_context->beginSingleTimeCommands();
    MC_ASSERT_RELEASE(cmd != VK_NULL_HANDLE);

    // 转换到传输布局
    transitionLayout(cmd,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    // 复制缓冲区到图像
    VkBufferImageCopy region{};
    region.bufferOffset = handle.offset;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = level;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {m_width, m_height, 1};

    vkCmdCopyBufferToImage(
        cmd, static_cast<VkBuffer>(pool->backingBuffer(0)), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // 转换到着色器只读布局
    transitionLayout(cmd,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    m_context->endSingleTimeCommands(cmd);
    pool->release(handle);

    return {};
}

Result<void> TridentTexture::uploadRegion(
    const void* data, u64 size, u32 offsetX, u32 offsetY, u32 width, u32 height, u32 level, u32 rowLength)
{
    if (m_image == VK_NULL_HANDLE || !m_context) {
        return Error(ErrorCode::OperationFailed, "Texture not initialized");
    }

    if (!data || size == 0) {
        return Error(ErrorCode::InvalidArgument, "Invalid data pointer or size");
    }

    // 验证区域参数
    if (offsetX + width > m_width || offsetY + height > m_height) {
        return Error(ErrorCode::OutOfRange, "Upload region out of texture bounds");
    }

    if (width == 0 || height == 0) {
        return Error(ErrorCode::InvalidArgument, "Region width and height must be greater than 0");
    }

    // 验证 mip level
    if (level >= m_mipLevels) {
        return Error(ErrorCode::OutOfRange, "Mip level out of range");
    }

    // 计算 mip level 下的实际偏移和尺寸
    const u32 mipOffsetX = offsetX >> level;
    const u32 mipOffsetY = offsetY >> level;
    const u32 mipWidth = width >> level;
    const u32 mipHeight = height >> level;

    // 通过统一暂存缓冲池上传子区域：stage → memcpy → 单次命令录制 buffer→image → release
    auto* pool = m_context->stagingPool();
    auto handle = pool->stage(size);
    MC_ASSERT_RELEASE_MSG(handle.valid, "TridentTexture::uploadRegion: staging pool out of space");
    std::memcpy(handle.mappedPtr, data, static_cast<size_t>(size));

    // 获取单次命令缓冲区
    VkCommandBuffer cmd = m_context->beginSingleTimeCommands();
    MC_ASSERT_RELEASE(cmd != VK_NULL_HANDLE);

    // 如果纹理当前不在传输目标布局，需要转换
    // 对于动画纹理更新，纹理应该已经在 SHADER_READ_ONLY_OPTIMAL 布局
    if (m_layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        transitionLayout(cmd,
            m_layout,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);
    }

    // 设置缓冲区到图像的复制区域
    // bufferRowLength 和 bufferImageHeight 实现源数据布局指定功能
    VkBufferImageCopy region{};
    region.bufferOffset = handle.offset;
    region.bufferRowLength = rowLength; // 0 表示使用 width（紧密排列）
    region.bufferImageHeight = 0;       // 对于 2D 纹理始终为 0
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = level;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {static_cast<i32>(mipOffsetX), static_cast<i32>(mipOffsetY), 0};
    region.imageExtent = {mipWidth, mipHeight, 1};

    vkCmdCopyBufferToImage(
        cmd, static_cast<VkBuffer>(pool->backingBuffer(0)), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // 转换回着色器只读布局
    transitionLayout(cmd,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    m_context->endSingleTimeCommands(cmd);
    pool->release(handle);

    return {};
}

void TridentTexture::bind(u32 binding)
{
    // 纹理绑定通常通过描述符集完成
    // 此方法留空，实际绑定在渲染管线中处理
}

void TridentTexture::transitionLayout(VkCommandBuffer cmd,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage)
{
    if (m_image == VK_NULL_HANDLE) return;

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = m_arrayLayers;

    // 设置访问掩码
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    } else {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    m_layout = newLayout;
}

Result<void> TridentTexture::generateMipmaps(VkCommandBuffer cmd)
{
    if (m_image == VK_NULL_HANDLE || !m_context) {
        return Error(ErrorCode::OperationFailed, "Texture not initialized");
    }

    if (m_mipLevels <= 1) {
        return {}; // 无需生成 mipmaps
    }

    // 检查设备是否支持线性过滤
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_context->physicalDevice(), m_vkFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        return Error(ErrorCode::Unsupported, "Texture format does not support linear blitting");
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = m_image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    i32 mipWidth = static_cast<i32>(m_width);
    i32 mipHeight = static_cast<i32>(m_height);

    for (u32 i = 1; i < m_mipLevels; ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(cmd,
            m_image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            m_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    return {};
}

Result<void> TridentTexture::createImage(VkImageUsageFlags usage, VkImageTiling tiling)
{
    VkDevice device = m_context->device();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_width;
    imageInfo.extent.height = m_height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = m_mipLevels;
    imageInfo.arrayLayers = m_arrayLayers;
    imageInfo.format = m_vkFormat;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    VkResult result = vkCreateImage(device, &imageInfo, nullptr, &m_image);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create image");
    }

    // 获取内存需求
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, m_image, &memRequirements);

    // 查找内存类型
    auto typeResult = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (typeResult.failed()) {
        vkDestroyImage(device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return typeResult.error();
    }

    // 分配内存
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = typeResult.value();

    result = vkAllocateMemory(device, &allocInfo, nullptr, &m_memory);
    if (result != VK_SUCCESS) {
        vkDestroyImage(device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return Error(ErrorCode::OutOfMemory, "Failed to allocate image memory");
    }

    vkBindImageMemory(device, m_image, m_memory, 0);
    m_layout = VK_IMAGE_LAYOUT_UNDEFINED;

    return {};
}

Result<void> TridentTexture::createImageView(VkImageAspectFlags aspect)
{
    VkDevice device = m_context->device();

    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_vkFormat;
    viewInfo.subresourceRange.aspectMask = aspect;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = m_arrayLayers;

    VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &m_imageView);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create image view");
    }

    return {};
}

Result<void> TridentTexture::createSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode)
{
    VkDevice device = m_context->device();

    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = magFilter;
    samplerInfo.minFilter = minFilter;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<f32>(m_mipLevels);

    VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &m_sampler);
    if (result != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create sampler");
    }

    return {};
}

Result<u32> TridentTexture::findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    return m_context->findMemoryType(typeFilter, properties);
}

VkFormat TridentTexture::toVkFormat(api::TextureFormat format)
{
    switch (format) {
        case api::TextureFormat::R8_UNORM:
            return VK_FORMAT_R8_UNORM;
        case api::TextureFormat::R8G8_UNORM:
            return VK_FORMAT_R8G8_UNORM;
        case api::TextureFormat::R8G8B8_UNORM:
            return VK_FORMAT_R8G8B8_UNORM;
        case api::TextureFormat::R8G8B8A8_UNORM:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case api::TextureFormat::R8G8B8_SRGB:
            return VK_FORMAT_R8G8B8_SRGB;
        case api::TextureFormat::R8G8B8A8_SRGB:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case api::TextureFormat::R16_FLOAT:
            return VK_FORMAT_R16_SFLOAT;
        case api::TextureFormat::R16G16_FLOAT:
            return VK_FORMAT_R16G16_SFLOAT;
        case api::TextureFormat::R16G16B16A16_FLOAT:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case api::TextureFormat::R32_FLOAT:
            return VK_FORMAT_R32_SFLOAT;
        case api::TextureFormat::R32G32_FLOAT:
            return VK_FORMAT_R32G32_SFLOAT;
        case api::TextureFormat::R32G32B32A32_FLOAT:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case api::TextureFormat::D16_UNORM:
            return VK_FORMAT_D16_UNORM;
        case api::TextureFormat::D24_UNORM_S8_UINT:
            return VK_FORMAT_D24_UNORM_S8_UINT;
        case api::TextureFormat::D32_FLOAT:
            return VK_FORMAT_D32_SFLOAT;
        case api::TextureFormat::BC1_RGB_SRGB:
            return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
        case api::TextureFormat::BC2_SRGB:
            return VK_FORMAT_BC2_SRGB_BLOCK;
        case api::TextureFormat::BC3_SRGB:
            return VK_FORMAT_BC3_SRGB_BLOCK;
        case api::TextureFormat::BC4_UNORM:
            return VK_FORMAT_BC4_UNORM_BLOCK;
        case api::TextureFormat::BC5_UNORM:
            return VK_FORMAT_BC5_UNORM_BLOCK;
        case api::TextureFormat::BC7_SRGB:
            return VK_FORMAT_BC7_SRGB_BLOCK;
        default:
            return VK_FORMAT_R8G8B8A8_SRGB;
    }
}

VkFilter TridentTexture::toVkFilter(api::TextureFilter filter)
{
    switch (filter) {
        case api::TextureFilter::Nearest:
        case api::TextureFilter::NearestMipmapNearest:
        case api::TextureFilter::NearestMipmapLinear:
            return VK_FILTER_NEAREST;
        case api::TextureFilter::Linear:
        case api::TextureFilter::LinearMipmapNearest:
        case api::TextureFilter::LinearMipmapLinear:
        default:
            return VK_FILTER_LINEAR;
    }
}

VkSamplerAddressMode TridentTexture::toVkAddressMode(api::TextureAddressMode mode)
{
    switch (mode) {
        case api::TextureAddressMode::Repeat:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case api::TextureAddressMode::MirroredRepeat:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case api::TextureAddressMode::ClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case api::TextureAddressMode::ClampToBorder:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case api::TextureAddressMode::MirrorClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
        default:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

// ============================================================================
// TridentTextureAtlas 实现
// ============================================================================

TridentTextureAtlas::TridentTextureAtlas() = default;

TridentTextureAtlas::~TridentTextureAtlas()
{
    destroy();
}

TridentTextureAtlas::TridentTextureAtlas(TridentTextureAtlas&& other) noexcept
    : m_context(other.m_context)
    , m_texture(std::move(other.m_texture))
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_tileSize(other.m_tileSize)
    , m_tilesPerRow(other.m_tilesPerRow)
    , m_tileU(other.m_tileU)
    , m_tileV(other.m_tileV)
{
    other.m_context = nullptr;
    other.m_width = 0;
    other.m_height = 0;
    other.m_tileSize = 0;
    other.m_tilesPerRow = 0;
    other.m_tileU = 0.0;
    other.m_tileV = 0.0;
}

TridentTextureAtlas& TridentTextureAtlas::operator=(TridentTextureAtlas&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_context = other.m_context;
        m_texture = std::move(other.m_texture);
        m_width = other.m_width;
        m_height = other.m_height;
        m_tileSize = other.m_tileSize;
        m_tilesPerRow = other.m_tilesPerRow;
        m_tileU = other.m_tileU;
        m_tileV = other.m_tileV;

        other.m_context = nullptr;
        other.m_width = 0;
        other.m_height = 0;
        other.m_tileSize = 0;
        other.m_tilesPerRow = 0;
        other.m_tileU = 0.0;
        other.m_tileV = 0.0;
    }
    return *this;
}

Result<void> TridentTextureAtlas::create(TridentContext* context, u32 width, u32 height, u32 tileSize)
{
    if (!context) {
        return Error(ErrorCode::NullPointer, "Context is null");
    }

    m_context = context;
    m_width = width;
    m_height = height;
    m_tileSize = tileSize;
    m_tilesPerRow = width / tileSize;
    m_tileU = 1.0 / static_cast<f64>(m_tilesPerRow);
    m_tileV = static_cast<f64>(tileSize) / static_cast<f64>(height);

    // 创建纹理
    api::TextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = api::TextureFormat::R8G8B8A8_UNORM; // 使用 UNORM 避免 sRGB 转换
    desc.mipLevels = 1;
    desc.generateMipmaps = false;
    desc.magFilter = api::TextureFilter::Nearest; // 像素风格使用最近邻过滤
    desc.minFilter = api::TextureFilter::Nearest;
    desc.addressModeU = api::TextureAddressMode::Repeat;
    desc.addressModeV = api::TextureAddressMode::Repeat;

    return m_texture.create(context, desc);
}

Result<void> TridentTextureAtlas::upload(const api::AtlasBuildResult& result)
{
    if (!isValid()) {
        return Error(ErrorCode::OperationFailed, "Atlas not initialized");
    }

    if (result.pixelData.empty()) {
        return Error(ErrorCode::InvalidArgument, "No pixel data in build result");
    }

    return m_texture.upload(result.pixelData.data(), result.pixelData.size(), 0);
}

void TridentTextureAtlas::destroy()
{
    m_texture.destroy();
    m_context = nullptr;
    m_width = 0;
    m_height = 0;
    m_tileSize = 0;
    m_tilesPerRow = 0;
    m_tileU = 0.0;
    m_tileV = 0.0;
}

api::TextureRegion TridentTextureAtlas::getRegion(u32 tileX, u32 tileY) const
{
    f64 u0 = static_cast<f64>(tileX * m_tileSize) / static_cast<f64>(m_width);
    f64 v0 = static_cast<f64>(tileY * m_tileSize) / static_cast<f64>(m_height);
    f64 u1 = static_cast<f64>((tileX + 1) * m_tileSize) / static_cast<f64>(m_width);
    f64 v1 = static_cast<f64>((tileY + 1) * m_tileSize) / static_cast<f64>(m_height);
    return api::TextureRegion(u0, v0, u1, v1);
}

api::TextureRegion TridentTextureAtlas::getRegion(u32 tileIndex) const
{
    u32 tileX = tileIndex % m_tilesPerRow;
    u32 tileY = tileIndex / m_tilesPerRow;
    return getRegion(tileX, tileY);
}

} // namespace mc::client::renderer::trident
