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

#include "LightTextureManager.hpp"
#include "client/renderer/trident/util/VulkanUtils.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::trident::light {

namespace {
constexpr u32 BYTES_PER_PIXEL = 4;
constexpr VkDeviceSize STAGING_SIZE = LIGHTMAP_SIZE * LIGHTMAP_SIZE * BYTES_PER_PIXEL;
} // namespace

LightTextureManager::LightTextureManager() = default;

LightTextureManager::~LightTextureManager()
{
    destroy();
}

Result<void> LightTextureManager::initialize(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    renderer::api::IStagingBufferPool* stagingPool)
{
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "LightTextureManager already initialized");
    }

    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_stagingPool = stagingPool;

    // 创建 16×16 RGBA8 光照贴图图像（采样目标）
    auto imageResult = VulkanUtils::createImage(m_device,
        m_physicalDevice,
        LIGHTMAP_SIZE,
        LIGHTMAP_SIZE,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_lightmapImage,
        m_lightmapMemory);
    if (!imageResult.success()) {
        return imageResult.error();
    }

    auto viewResult = VulkanUtils::createImageView(
        m_device, m_lightmapImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, m_lightmapView);
    if (!viewResult.success()) {
        return viewResult.error();
    }

    // 采样器：lightmap 用 CLAMP_TO_EDGE，线性过滤
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create lightmap sampler");
    }

    // 首次上传：UNDEFINED → TRANSFER_DST → SHADER_READ_ONLY（同步路径，用 stagingPool 的 stage/release）
    LightmapInputs defaultInputs{};
    updateLightTexture(defaultInputs);

    m_initialized = true;
    return {};
}

void LightTextureManager::destroy()
{
    if (m_device == VK_NULL_HANDLE) {
        return;
    }

    vkDeviceWaitIdle(m_device);

    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }
    if (m_lightmapView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_lightmapView, nullptr);
        m_lightmapView = VK_NULL_HANDLE;
    }
    if (m_lightmapImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_lightmapImage, nullptr);
        m_lightmapImage = VK_NULL_HANDLE;
    }
    if (m_lightmapMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_lightmapMemory, nullptr);
        m_lightmapMemory = VK_NULL_HANDLE;
    }

    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_graphicsQueue = VK_NULL_HANDLE;
    m_stagingPool = nullptr;
    m_initialized = false;
}

glm::vec3 LightTextureManager::_computePixel(u32 blockLight, u32 skyLight, const LightmapInputs& inputs)
{
    const f32 blockF = static_cast<f32>(blockLight) / 15.0f;
    const f32 skyF = static_cast<f32>(skyLight) / 15.0f;

    // 天空光通道：天空色 × 天空光因子 × 天空光比例
    const glm::vec3 skyChannel = inputs.skyLightColor * inputs.skyLightFactor * skyF;
    // 方块光通道：火光近似暖白（不受天空遮挡）
    const glm::vec3 blockChannel = glm::vec3(1.0f) * blockF;

    // 合并：方块光与天空光取 max
    glm::vec3 combined = glm::max(skyChannel, blockChannel);

    // getBrightness 曲线：level/(4-3*level) 非线性提亮（逐通道，clamp 防 4-3x→0）
    combined = glm::clamp(combined, 0.0f, 0.99f);
    glm::vec3 brightness = combined / (4.0f - 3.0f * combined);

    // 维度环境光下限
    brightness = glm::max(brightness, glm::vec3(inputs.ambientLight));

    // gamma 调制（简化：gamma 提升暗部）
    const f32 gammaExp = 1.0f - std::clamp(inputs.gamma, 0.0f, 1.0f) * 0.5f;
    brightness = glm::pow(brightness, glm::vec3(gammaExp));

    // 夜视：拉高整体下限（TODO：接入玩家 NIGHT_VISION 后细化）
    if (inputs.nightVision > 0.0f) {
        brightness = glm::max(brightness, glm::vec3(inputs.nightVision * 0.5f));
    }
    // 黑暗：压低亮度（TODO：接入玩家 DARKNESS 后细化）
    brightness *= (1.0f - std::clamp(inputs.darkness, 0.0f, 1.0f));

    // 闪电闪烁增亮
    const f32 flash = std::clamp(inputs.darkenWorldAmount, 0.0f, 1.0f) * 0.5f;
    brightness = glm::mix(brightness, glm::vec3(1.0f), flash);

    // 末地闪烁（TODO：末地 endFlash 接入后细化）
    brightness = glm::mix(brightness, glm::vec3(1.0f), std::clamp(inputs.endFlash, 0.0f, 1.0f) * 0.3f);

    return glm::clamp(brightness, 0.0f, 1.0f);
}

void LightTextureManager::_transitionLayout(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_lightmapImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage = 0;
    VkPipelineStageFlags dstStage = 0;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else {
        spdlog::warn("LightTextureManager: unsupported layout transition {} -> {}",
            static_cast<int>(oldLayout),
            static_cast<int>(newLayout));
        return;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void LightTextureManager::_computePixels(const LightmapInputs& inputs)
{
    for (u32 sky = 0; sky < LIGHTMAP_SIZE; ++sky) {
        for (u32 block = 0; block < LIGHTMAP_SIZE; ++block) {
            const glm::vec3 color = _computePixel(block, sky, inputs);
            const u32 idx = (sky * LIGHTMAP_SIZE + block) * BYTES_PER_PIXEL;
            m_pixels[idx + 0] = static_cast<u8>(color.r * 255.0f + 0.5f);
            m_pixels[idx + 1] = static_cast<u8>(color.g * 255.0f + 0.5f);
            m_pixels[idx + 2] = static_cast<u8>(color.b * 255.0f + 0.5f);
            m_pixels[idx + 3] = 255;
        }
    }
}

void LightTextureManager::_recordUpload(VkCommandBuffer cmd, VkBuffer stagingBuffer, VkDeviceSize stagingOffset)
{
    // 首次（m_initialized=false）用 UNDEFINED→TRANSFER_DST；后续用 SHADER_READ_ONLY→TRANSFER_DST
    const VkImageLayout oldLayout =
        m_initialized ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
    _transitionLayout(cmd, oldLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region{};
    region.bufferOffset = stagingOffset;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {LIGHTMAP_SIZE, LIGHTMAP_SIZE, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, m_lightmapImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    _transitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void LightTextureManager::updateLightTexture(const LightmapInputs& inputs)
{
    if (m_device == VK_NULL_HANDLE || m_stagingPool == nullptr) {
        return;
    }

    _computePixels(inputs);

    // 同步路径：stage 子分配 + memcpy + 独立命令缓冲 submit+wait + release。
    // 仅 initialize 首帧使用；每帧热路径用 updateLightTextureAsync。
    auto handle = m_stagingPool->stage(STAGING_SIZE);
    if (!handle.valid) {
        spdlog::warn("LightTextureManager::updateLightTexture: staging pool out of space");
        return;
    }
    std::memcpy(handle.mappedPtr, m_pixels.data(), static_cast<size_t>(STAGING_SIZE));

    VkCommandBuffer cmd = VulkanUtils::beginSingleTimeCommands(m_device, m_commandPool);
    _recordUpload(cmd, static_cast<VkBuffer>(m_stagingPool->backingBuffer(0)), handle.offset);
    VulkanUtils::endSingleTimeCommands(m_device, m_commandPool, m_graphicsQueue, cmd);

    m_stagingPool->release(handle);
}

void LightTextureManager::updateLightTextureAsync(const LightmapInputs& inputs, VkCommandBuffer cmd, u32 frameIndex)
{
    if (m_device == VK_NULL_HANDLE || m_stagingPool == nullptr) {
        return;
    }
    MC_ASSERT_RELEASE_MSG(cmd != VK_NULL_HANDLE, "LightTextureManager::updateLightTextureAsync: cmd must be valid");

    _computePixels(inputs);

    // 异步路径：stageAsync 子分配暂存区间（登记到 frameIndex 回收桶，由 recycleFrame 回收），
    // memcpy 后把布局转换 + copy 录进帧命令缓冲，随帧 submit、用帧 fence 同步，不阻塞 CPU。
    auto handle = m_stagingPool->stageAsync(STAGING_SIZE, frameIndex);
    if (!handle.valid) {
        spdlog::warn("LightTextureManager::updateLightTextureAsync: staging pool out of space, lightmap skipped");
        return;
    }
    std::memcpy(handle.mappedPtr, m_pixels.data(), static_cast<size_t>(STAGING_SIZE));

    _recordUpload(cmd, static_cast<VkBuffer>(m_stagingPool->backingBuffer(0)), handle.offset);
    // 不 release：区间由 recycleFrame 在下一轮同 slot 回收
}

} // namespace mc::client::renderer::trident::light
