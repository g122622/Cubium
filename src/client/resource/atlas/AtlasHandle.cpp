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

#include "client/resource/atlas/AtlasHandle.hpp"

#include "client/renderer/api/buffer/IStagingBufferPool.hpp"
#include "client/renderer/trident/util/VulkanUtils.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <cstdint>
#include <cstring>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace mc::client::resource::atlas {

AtlasHandle::~AtlasHandle()
{
    destroy();
}

AtlasHandle::AtlasHandle(AtlasHandle&& other) noexcept
    : m_device(other.m_device)
    , m_physicalDevice(other.m_physicalDevice)
    , m_commandPool(other.m_commandPool)
    , m_graphicsQueue(other.m_graphicsQueue)
    , m_stagingPool(other.m_stagingPool)
    , m_image(other.m_image)
    , m_imageMemory(other.m_imageMemory)
    , m_imageView(other.m_imageView)
    , m_sampler(other.m_sampler)
    , m_filter(other.m_filter)
    , m_imageWidth(other.m_imageWidth)
    , m_imageHeight(other.m_imageHeight)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_uploaded(other.m_uploaded)
{
    other.m_device = VK_NULL_HANDLE;
    other.m_physicalDevice = VK_NULL_HANDLE;
    other.m_commandPool = VK_NULL_HANDLE;
    other.m_graphicsQueue = VK_NULL_HANDLE;
    other.m_stagingPool = nullptr;
    other.m_image = VK_NULL_HANDLE;
    other.m_imageMemory = VK_NULL_HANDLE;
    other.m_imageView = VK_NULL_HANDLE;
    other.m_sampler = VK_NULL_HANDLE;
    other.m_imageWidth = 0;
    other.m_imageHeight = 0;
    other.m_width = 0;
    other.m_height = 0;
    other.m_uploaded = false;
}

AtlasHandle& AtlasHandle::operator=(AtlasHandle&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_device = other.m_device;
        m_physicalDevice = other.m_physicalDevice;
        m_commandPool = other.m_commandPool;
        m_graphicsQueue = other.m_graphicsQueue;
        m_stagingPool = other.m_stagingPool;
        m_image = other.m_image;
        m_imageMemory = other.m_imageMemory;
        m_imageView = other.m_imageView;
        m_sampler = other.m_sampler;
        m_filter = other.m_filter;
        m_imageWidth = other.m_imageWidth;
        m_imageHeight = other.m_imageHeight;
        m_width = other.m_width;
        m_height = other.m_height;
        m_uploaded = other.m_uploaded;

        other.m_device = VK_NULL_HANDLE;
        other.m_physicalDevice = VK_NULL_HANDLE;
        other.m_commandPool = VK_NULL_HANDLE;
        other.m_graphicsQueue = VK_NULL_HANDLE;
        other.m_stagingPool = nullptr;
        other.m_image = VK_NULL_HANDLE;
        other.m_imageMemory = VK_NULL_HANDLE;
        other.m_imageView = VK_NULL_HANDLE;
        other.m_sampler = VK_NULL_HANDLE;
        other.m_imageWidth = 0;
        other.m_imageHeight = 0;
        other.m_width = 0;
        other.m_height = 0;
        other.m_uploaded = false;
    }
    return *this;
}

Result<void> AtlasHandle::create(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    renderer::api::IStagingBufferPool* stagingPool,
    u32 width,
    u32 height,
    VkFilter filter)
{
    if (device == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "AtlasHandle: device is null");
    }
    if (commandPool == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "AtlasHandle: command pool is null");
    }
    if (graphicsQueue == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "AtlasHandle: graphics queue is null");
    }
    // 暂存池是所有像素上传的唯一通道，缺失说明初始化顺序错误
    MC_ASSERT_RELEASE_MSG(stagingPool != nullptr, "AtlasHandle::create: staging pool must be injected");
    if (width == 0 || height == 0) {
        return Error(ErrorCode::InvalidArgument, "AtlasHandle: dimensions must be non-zero");
    }

    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_stagingPool = stagingPool;
    m_width = width;
    m_height = height;
    m_filter = filter;

    auto imageResult = _createImage();
    if (!imageResult.success()) {
        return imageResult.error();
    }

    auto viewResult = _createImageView();
    if (!viewResult.success()) {
        vkDestroyImage(m_device, m_image, nullptr);
        vkFreeMemory(m_device, m_imageMemory, nullptr);
        m_image = VK_NULL_HANDLE;
        m_imageMemory = VK_NULL_HANDLE;
        return viewResult.error();
    }

    auto samplerResult = _createSampler();
    if (!samplerResult.success()) {
        vkDestroyImageView(m_device, m_imageView, nullptr);
        vkDestroyImage(m_device, m_image, nullptr);
        vkFreeMemory(m_device, m_imageMemory, nullptr);
        m_imageView = VK_NULL_HANDLE;
        m_image = VK_NULL_HANDLE;
        m_imageMemory = VK_NULL_HANDLE;
        return samplerResult.error();
    }

    return {};
}

void AtlasHandle::destroy()
{
    if (m_sampler != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }
    if (m_imageView != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }
    if (m_image != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
    }
    if (m_imageMemory != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_imageMemory, nullptr);
        m_imageMemory = VK_NULL_HANDLE;
    }

    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_graphicsQueue = VK_NULL_HANDLE;
    m_stagingPool = nullptr;
    m_imageWidth = 0;
    m_imageHeight = 0;
    m_width = 0;
    m_height = 0;
    m_uploaded = false;
}

Result<void> AtlasHandle::upload(const void* pixels, u64 size, u32 width, u32 height)
{
    if (pixels == nullptr) {
        return Error(ErrorCode::NullPointer, "AtlasHandle::upload: pixels is null");
    }
    if (width == 0 || height == 0) {
        return Error(ErrorCode::InvalidArgument, "AtlasHandle::upload: dimensions must be non-zero");
    }

    m_width = width;
    m_height = height;

    const bool needRecreateImage =
        (m_image == VK_NULL_HANDLE) || (m_imageWidth != m_width) || (m_imageHeight != m_height);

    if (needRecreateImage) {
        if (m_imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_device, m_imageView, nullptr);
            m_imageView = VK_NULL_HANDLE;
        }
        if (m_image != VK_NULL_HANDLE) {
            vkDestroyImage(m_device, m_image, nullptr);
            m_image = VK_NULL_HANDLE;
        }
        if (m_imageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, m_imageMemory, nullptr);
            m_imageMemory = VK_NULL_HANDLE;
        }

        auto imageResult = _createImage();
        if (!imageResult.success()) {
            return imageResult.error();
        }
        auto viewResult = _createImageView();
        if (!viewResult.success()) {
            vkDestroyImage(m_device, m_image, nullptr);
            vkFreeMemory(m_device, m_imageMemory, nullptr);
            m_image = VK_NULL_HANDLE;
            m_imageMemory = VK_NULL_HANDLE;
            return viewResult.error();
        }
    }

    // 经统一暂存池上传：stage → memcpy → 单次命令缓冲 buffer→image → release
    auto handle = m_stagingPool->stage(size);
    MC_ASSERT_RELEASE_MSG(handle.valid, "AtlasHandle::upload: staging pool out of space");
    std::memcpy(handle.mappedPtr, pixels, static_cast<size_t>(size));

    const VkImageLayout uploadOldLayout =
        (needRecreateImage || !m_uploaded) ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    const VkPipelineStageFlags uploadSrcStage =
        (needRecreateImage || !m_uploaded) ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    VkCommandBuffer cmd = _beginSingleTimeCommands();

    _transitionImageLayout(
        cmd, uploadOldLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, uploadSrcStage, VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkBufferImageCopy region{};
    region.bufferOffset = handle.offset;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {m_width, m_height, 1};

    vkCmdCopyBufferToImage(cmd,
        static_cast<VkBuffer>(m_stagingPool->backingBuffer(0)),
        m_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    _transitionImageLayout(cmd,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    _endSingleTimeCommands(cmd);
    // submit+wait 已在 endSingleTimeCommands 内完成，staging 区间可安全归还
    m_stagingPool->release(handle);

    m_uploaded = true;
    return {};
}

Result<void> AtlasHandle::uploadRegion(
    const void* pixelData, u64 size, u32 offsetX, u32 offsetY, u32 width, u32 height, u32 rowLength)
{
    if (pixelData == nullptr) {
        return Error(ErrorCode::NullPointer, "AtlasHandle::uploadRegion: pixel data is null");
    }
    if (m_image == VK_NULL_HANDLE) {
        return Error(ErrorCode::InvalidState, "AtlasHandle::uploadRegion: atlas not initialized");
    }
    if (width == 0 || height == 0) {
        return Error(ErrorCode::InvalidArgument, "AtlasHandle::uploadRegion: dimensions must be non-zero");
    }

    // 经统一暂存池子分配，单次命令缓冲提交
    auto handle = m_stagingPool->stage(size);
    MC_ASSERT_RELEASE_MSG(handle.valid, "AtlasHandle::uploadRegion: staging pool out of space");
    std::memcpy(handle.mappedPtr, pixelData, static_cast<size_t>(size));

    VkCommandBuffer cmd = _beginSingleTimeCommands();

    _transitionImageLayout(cmd,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkBufferImageCopy region{};
    region.bufferOffset = handle.offset;
    region.bufferRowLength = rowLength > 0 ? rowLength : width;
    region.bufferImageHeight = height;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {static_cast<int32_t>(offsetX), static_cast<int32_t>(offsetY), 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmd,
        static_cast<VkBuffer>(m_stagingPool->backingBuffer(0)),
        m_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    _transitionImageLayout(cmd,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    _endSingleTimeCommands(cmd);
    m_stagingPool->release(handle);

    return {};
}

Result<void> AtlasHandle::uploadRegionsBatch(const std::vector<RegionUpload>& regions)
{
    if (regions.empty()) {
        return {};
    }
    if (m_image == VK_NULL_HANDLE) {
        return Error(ErrorCode::InvalidState, "AtlasHandle::uploadRegionsBatch: atlas not initialized");
    }

    // 1. 先把所有区域子分配到同一 backing buffer，各自 memcpy 源像素
    //    记录每个区域的 staging offset，供后续录制 vkCmdCopyBufferToImage 复用
    struct StagedRegion {
        renderer::api::StagingHandle handle;
        const RegionUpload* desc;
    };
    std::vector<StagedRegion> staged;
    staged.reserve(regions.size());

    for (const auto& r : regions) {
        if (r.pixelData == nullptr || r.width == 0 || r.height == 0) {
            // 整批一致性要求：任一区域非法即放弃整批，先释放已分配的
            for (const auto& s : staged) {
                m_stagingPool->release(s.handle);
            }
            return Error(ErrorCode::InvalidArgument, "AtlasHandle::uploadRegionsBatch: invalid region in batch");
        }
        auto handle = m_stagingPool->stage(r.size);
        if (!handle.valid) {
            // 池容量不足：释放已分配区间，调用方应在下一帧重试或走降频
            for (const auto& s : staged) {
                m_stagingPool->release(s.handle);
            }
            return Error(ErrorCode::OutOfMemory, "AtlasHandle::uploadRegionsBatch: staging pool out of space");
        }
        std::memcpy(handle.mappedPtr, r.pixelData, static_cast<size_t>(r.size));
        staged.push_back({handle, &r});
    }

    // 2. 单个命令缓冲：一次 layout 转入 + N 次 copy + 一次 layout 转出，一次 submit+wait
    VkCommandBuffer cmd = _beginSingleTimeCommands();

    _transitionImageLayout(cmd,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    for (const auto& s : staged) {
        VkBufferImageCopy region{};
        region.bufferOffset = s.handle.offset;
        region.bufferRowLength = s.desc->rowLength > 0 ? s.desc->rowLength : s.desc->width;
        region.bufferImageHeight = s.desc->height;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {static_cast<int32_t>(s.desc->offsetX), static_cast<int32_t>(s.desc->offsetY), 0};
        region.imageExtent = {s.desc->width, s.desc->height, 1};

        vkCmdCopyBufferToImage(cmd,
            static_cast<VkBuffer>(m_stagingPool->backingBuffer(0)),
            m_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);
    }

    _transitionImageLayout(cmd,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    _endSingleTimeCommands(cmd);

    // 3. submit+wait 完成，统一归还所有 staging 区间
    for (const auto& s : staged) {
        m_stagingPool->release(s.handle);
    }

    return {};
}

Result<void> AtlasHandle::uploadRegionsBatchAsync(
    const std::vector<RegionUpload>& regions, VkCommandBuffer cmd, u32 frameIndex)
{
    if (regions.empty()) {
        return {};
    }
    if (m_image == VK_NULL_HANDLE) {
        return Error(ErrorCode::InvalidState, "AtlasHandle::uploadRegionsBatchAsync: atlas not initialized");
    }
    MC_ASSERT_RELEASE_MSG(cmd != VK_NULL_HANDLE, "AtlasHandle::uploadRegionsBatchAsync: cmd must be valid");

    // 1. stageAsync 把所有区域子分配到同一 backing buffer，各自 memcpy 源像素
    //    分配的区间登记到 frameIndex 回收桶，由 stagingPool 在下一轮同 slot 的
    //    recycleFrame 回收，调用方不 release。失败区间不会登记（无需回滚）。
    struct StagedRegion {
        renderer::api::StagingHandle handle;
        const RegionUpload* desc;
    };
    std::vector<StagedRegion> staged;
    staged.reserve(regions.size());

    for (const auto& r : regions) {
        if (r.pixelData == nullptr || r.width == 0 || r.height == 0) {
            return Error(ErrorCode::InvalidArgument, "AtlasHandle::uploadRegionsBatchAsync: invalid region in batch");
        }
        auto handle = m_stagingPool->stageAsync(r.size, frameIndex);
        if (!handle.valid) {
            // 池容量不足：已 stageAsync 成功的区间已登记到桶，由 recycleFrame 自动回收，不泄漏。
            // 未上传成功的 sprite 由调用方下帧重试。
            return Error(ErrorCode::OutOfMemory, "AtlasHandle::uploadRegionsBatchAsync: staging pool out of space");
        }
        std::memcpy(handle.mappedPtr, r.pixelData, static_cast<size_t>(r.size));
        staged.push_back({handle, &r});
    }

    // 2. 录进调用方提供的帧命令缓冲：一次 layout 转入 + N 次 copy + 一次 layout 转出
    //    atlas 长期处于 SHADER_READ_ONLY，oldLayout 恒为 SHADER_READ_ONLY_OPTIMAL。
    _transitionImageLayout(cmd,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    for (const auto& s : staged) {
        VkBufferImageCopy region{};
        region.bufferOffset = s.handle.offset;
        region.bufferRowLength = s.desc->rowLength > 0 ? s.desc->rowLength : s.desc->width;
        region.bufferImageHeight = s.desc->height;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {static_cast<int32_t>(s.desc->offsetX), static_cast<int32_t>(s.desc->offsetY), 0};
        region.imageExtent = {s.desc->width, s.desc->height, 1};

        vkCmdCopyBufferToImage(cmd,
            static_cast<VkBuffer>(m_stagingPool->backingBuffer(0)),
            m_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);
    }

    _transitionImageLayout(cmd,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    // 3. 不 submit、不 release：命令随帧命令缓冲 submit，区间由 recycleFrame 回收
    return {};
}

Result<void> AtlasHandle::_createImage()
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_width;
    imageInfo.extent.height = m_height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "AtlasHandle: failed to create atlas image");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, m_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    auto memTypeResult = _findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (!memTypeResult.success()) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return memTypeResult.error();
    }
    allocInfo.memoryTypeIndex = memTypeResult.value();

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return Error(ErrorCode::OutOfMemory, "AtlasHandle: failed to allocate atlas memory");
    }

    vkBindImageMemory(m_device, m_image, m_imageMemory, 0);
    m_imageWidth = m_width;
    m_imageHeight = m_height;

    return {};
}

Result<void> AtlasHandle::_createSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = m_filter;
    samplerInfo.minFilter = m_filter;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "AtlasHandle: failed to create atlas sampler");
    }
    return {};
}

Result<void> AtlasHandle::_createImageView()
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "AtlasHandle: failed to create atlas image view");
    }
    return {};
}

Result<u32> AtlasHandle::_findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    return renderer::VulkanUtils::findMemoryType(m_physicalDevice, typeFilter, properties);
}

VkCommandBuffer AtlasHandle::_beginSingleTimeCommands()
{
    return renderer::VulkanUtils::beginSingleTimeCommands(m_device, m_commandPool);
}

void AtlasHandle::_endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    renderer::VulkanUtils::endSingleTimeCommands(m_device, m_commandPool, m_graphicsQueue, commandBuffer);
}

void AtlasHandle::_transitionImageLayout(VkCommandBuffer cmd,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage)
{
    renderer::VulkanUtils::transitionImageLayout(cmd, m_image, oldLayout, newLayout, srcStage, dstStage);
}

} // namespace mc::client::resource::atlas
