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

#include "GuiSpriteAtlas.hpp"
#include "GuiSpriteManager.hpp"
#include "client/renderer/trident/gui/GuiSprite.hpp"
#include "client/ui/kagero/paint/TextureImage.hpp"
#include "client/ui/kagero/paint/contracts/IImage.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident::gui {

// ============================================================================
// Pimpl实现
// ============================================================================

class GuiSpriteAtlas::Impl {
public:
    Impl() = default;
    ~Impl() = default;

    // Vulkan资源
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;

    // 纹理资源
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;

    // 图集尺寸
    u32 width = 256;
    u32 height = 256;

    // 精灵管理
    GuiSpriteManager spriteManager;

    // 纹理数据缓存（用于延迟上传）
    std::vector<u8> textureData;

    bool initialized = false;
};

// ============================================================================
// 构造函数 / 析构函数
// ============================================================================

GuiSpriteAtlas::GuiSpriteAtlas()
    : m_impl(std::make_unique<Impl>())
{}

GuiSpriteAtlas::~GuiSpriteAtlas()
{
    destroy();
}

GuiSpriteAtlas::GuiSpriteAtlas(GuiSpriteAtlas&& other) noexcept
    : m_impl(std::move(other.m_impl))
    , m_initialized(other.m_initialized)
{
    other.m_initialized = false;
}

GuiSpriteAtlas& GuiSpriteAtlas::operator=(GuiSpriteAtlas&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_impl = std::move(other.m_impl);
        m_initialized = other.m_initialized;
        other.m_initialized = false;
    }
    return *this;
}

// ============================================================================
// 初始化与销毁
// ============================================================================

Result<void> GuiSpriteAtlas::initialize(
    VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue)
{
    if (m_initialized) {
        return {};
    }

    if (device == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "Device is null");
    }
    if (commandPool == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "Command pool is null");
    }
    if (graphicsQueue == VK_NULL_HANDLE) {
        return Error(ErrorCode::NullPointer, "Graphics queue is null");
    }

    m_impl->device = device;
    m_impl->physicalDevice = physicalDevice;
    m_impl->commandPool = commandPool;
    m_impl->graphicsQueue = graphicsQueue;

    // 创建默认纹理数据
    m_impl->textureData.resize(m_impl->width * m_impl->height * 4, 0);

    // 创建默认采样器（使用最近邻过滤以保持像素清晰）
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &m_impl->sampler) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create GUI sprite atlas sampler");
    }

    m_initialized = true;
    return {};
}

void GuiSpriteAtlas::destroy()
{
    if (!m_initialized) {
        return;
    }

    if (m_impl->sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_impl->device, m_impl->sampler, nullptr);
        m_impl->sampler = VK_NULL_HANDLE;
    }

    if (m_impl->imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_impl->device, m_impl->imageView, nullptr);
        m_impl->imageView = VK_NULL_HANDLE;
    }

    if (m_impl->image != VK_NULL_HANDLE) {
        vkDestroyImage(m_impl->device, m_impl->image, nullptr);
        m_impl->image = VK_NULL_HANDLE;
    }

    if (m_impl->imageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_impl->device, m_impl->imageMemory, nullptr);
        m_impl->imageMemory = VK_NULL_HANDLE;
    }

    m_impl->spriteManager.clearSprites();
    m_impl->textureData.clear();

    m_impl->device = VK_NULL_HANDLE;
    m_impl->physicalDevice = VK_NULL_HANDLE;
    m_impl->commandPool = VK_NULL_HANDLE;
    m_impl->graphicsQueue = VK_NULL_HANDLE;

    m_initialized = false;
}

// ============================================================================
// 纹理加载
// ============================================================================

Result<void> GuiSpriteAtlas::loadTextureFromMemory(const std::vector<u8>& pixels, i32 width, i32 height)
{
    if (!m_initialized) {
        return Error(ErrorCode::InitializationFailed, "GuiSpriteAtlas not initialized");
    }

    if (pixels.empty()) {
        return Error(ErrorCode::InvalidArgument, "Pixel data is empty");
    }

    if (width <= 0 || height <= 0) {
        return Error(ErrorCode::InvalidArgument, "Invalid texture dimensions");
    }

    // 验证像素数据大小
    const size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    if (pixels.size() != expectedSize) {
        return Error(ErrorCode::InvalidArgument,
            std::string("Pixel data size mismatch: expected ") + std::to_string(expectedSize) + " bytes, got " +
                std::to_string(pixels.size()) + " bytes");
    }

    // 销毁旧纹理（如果存在）
    if (m_impl->imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_impl->device, m_impl->imageView, nullptr);
        m_impl->imageView = VK_NULL_HANDLE;
    }
    if (m_impl->image != VK_NULL_HANDLE) {
        vkDestroyImage(m_impl->device, m_impl->image, nullptr);
        m_impl->image = VK_NULL_HANDLE;
    }
    if (m_impl->imageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_impl->device, m_impl->imageMemory, nullptr);
        m_impl->imageMemory = VK_NULL_HANDLE;
    }

    // 更新图集尺寸
    m_impl->width = static_cast<u32>(width);
    m_impl->height = static_cast<u32>(height);
    m_impl->spriteManager.setAtlasSize(width, height);

    // 创建Vulkan图像
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<u32>(width);
    imageInfo.extent.height = static_cast<u32>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(m_impl->device, &imageInfo, nullptr, &m_impl->image) != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create GUI sprite atlas image");
    }

    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_impl->device, m_impl->image, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_impl->physicalDevice, &memProperties);

    u32 memoryTypeIndex = UINT32_MAX;
    for (u32 i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            memoryTypeIndex = i;
            break;
        }
    }

    if (memoryTypeIndex == UINT32_MAX) {
        vkDestroyImage(m_impl->device, m_impl->image, nullptr);
        m_impl->image = VK_NULL_HANDLE;
        return Error(ErrorCode::OutOfMemory, "Failed to find suitable memory type");
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(m_impl->device, &allocInfo, nullptr, &m_impl->imageMemory) != VK_SUCCESS) {
        vkDestroyImage(m_impl->device, m_impl->image, nullptr);
        m_impl->image = VK_NULL_HANDLE;
        return Error(ErrorCode::OutOfMemory, "Failed to allocate GUI sprite atlas memory");
    }

    vkBindImageMemory(m_impl->device, m_impl->image, m_impl->imageMemory, 0);

    // 创建图像视图
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_impl->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_impl->device, &viewInfo, nullptr, &m_impl->imageView) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create GUI sprite atlas image view");
    }

    // 上传纹理数据到GPU
    VkDeviceSize imageSize = pixels.size();

    // 创建暂存缓冲区
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_impl->device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create staging buffer");
    }

    vkGetBufferMemoryRequirements(m_impl->device, stagingBuffer, &memRequirements);

    memoryTypeIndex = UINT32_MAX;
    for (u32 i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags &
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
            memoryTypeIndex = i;
            break;
        }
    }

    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(m_impl->device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        vkDestroyBuffer(m_impl->device, stagingBuffer, nullptr);
        return Error(ErrorCode::OutOfMemory, "Failed to allocate staging memory");
    }

    vkBindBufferMemory(m_impl->device, stagingBuffer, stagingMemory, 0);

    // 复制数据到暂存缓冲区
    void* mappedData = nullptr;
    if (vkMapMemory(m_impl->device, stagingMemory, 0, imageSize, 0, &mappedData) != VK_SUCCESS) {
        vkDestroyBuffer(m_impl->device, stagingBuffer, nullptr);
        vkFreeMemory(m_impl->device, stagingMemory, nullptr);
        return Error(ErrorCode::OperationFailed, "Failed to map staging memory");
    }
    std::memcpy(mappedData, pixels.data(), pixels.size());
    vkUnmapMemory(m_impl->device, stagingMemory);

    // 录制命令缓冲区
    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = m_impl->commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(m_impl->device, &cmdAllocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);

    // 转换图像布局到传输目标
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_impl->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // 复制缓冲区到图像
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {m_impl->width, m_impl->height, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, m_impl->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // 转换图像布局到着色器只读
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

    vkEndCommandBuffer(cmd);

    // 提交命令
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(m_impl->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_impl->graphicsQueue);

    // 清理暂存缓冲区
    vkFreeCommandBuffers(m_impl->device, m_impl->commandPool, 1, &cmd);
    vkDestroyBuffer(m_impl->device, stagingBuffer, nullptr);
    vkFreeMemory(m_impl->device, stagingMemory, nullptr);

    return {};
}

bool GuiSpriteAtlas::hasTexture() const
{
    return m_impl->image != VK_NULL_HANDLE && m_impl->imageView != VK_NULL_HANDLE;
}

Result<void> GuiSpriteAtlas::loadDefaultTextures()
{
    if (!m_initialized) {
        return Error(ErrorCode::InitializationFailed, "GuiSpriteAtlas not initialized");
    }

    // 如果纹理已存在，不再创建
    if (m_impl->image != VK_NULL_HANDLE) {
        return {};
    }

    // 创建默认GUI图集纹理（256x256 RGBA）
    constexpr u32 DEFAULT_SIZE = 256;
    m_impl->width = DEFAULT_SIZE;
    m_impl->height = DEFAULT_SIZE;
    m_impl->textureData.resize(DEFAULT_SIZE * DEFAULT_SIZE * 4, 0);

    // 创建棋盘格占位图案（用于调试）
    for (u32 y = 0; y < DEFAULT_SIZE; ++y) {
        for (u32 x = 0; x < DEFAULT_SIZE; ++x) {
            u32 idx = (y * DEFAULT_SIZE + x) * 4;
            bool checker = ((x / 16) + (y / 16)) % 2 == 0;
            if (checker) {
                m_impl->textureData[idx + 0] = 128; // R
                m_impl->textureData[idx + 1] = 128; // G
                m_impl->textureData[idx + 2] = 128; // B
                m_impl->textureData[idx + 3] = 255; // A
            } else {
                m_impl->textureData[idx + 0] = 64;  // R
                m_impl->textureData[idx + 1] = 64;  // G
                m_impl->textureData[idx + 2] = 64;  // B
                m_impl->textureData[idx + 3] = 255; // A
            }
        }
    }

    return loadTextureFromMemory(m_impl->textureData, DEFAULT_SIZE, DEFAULT_SIZE);
}

Result<void> GuiSpriteAtlas::loadTextureAtlas(const std::string& filePath, i32 atlasWidth, i32 atlasHeight)
{
    if (!m_initialized) {
        return Error(ErrorCode::InitializationFailed, "GuiSpriteAtlas not initialized");
    }

    // 使用stb_image加载PNG文件
    int width, height, channels;
    u8* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, 4);

    if (!pixels) {
        return Error(ErrorCode::TextureLoadFailed,
            std::string("Failed to load texture: ") + filePath + " - " + stbi_failure_reason());
    }

    // 复制像素数据
    std::vector<u8> pixelData(pixels, pixels + static_cast<size_t>(width * height * 4));
    stbi_image_free(pixels);

    // 如果指定了图集尺寸，使用指定的；否则使用实际图像尺寸
    i32 texWidth = (atlasWidth > 0) ? atlasWidth : width;
    i32 texHeight = (atlasHeight > 0) ? atlasHeight : height;

    auto result = loadTextureFromMemory(pixelData, texWidth, texHeight);
    if (result.failed()) {
        return result;
    }

    // 保存纹理数据缓存
    m_impl->textureData = std::move(pixelData);

    return {};
}

// ============================================================================
// 精灵管理
// ============================================================================

void GuiSpriteAtlas::registerSprite(const GuiSprite& sprite)
{
    m_impl->spriteManager.registerSprite(sprite);
}

void GuiSpriteAtlas::registerSprite(
    const std::string& id, i32 x, i32 y, i32 width, i32 height, i32 atlasWidth, i32 atlasHeight)
{
    // 使用默认图集尺寸（如果未指定）
    i32 aw = (atlasWidth > 0) ? atlasWidth : m_impl->width;
    i32 ah = (atlasHeight > 0) ? atlasHeight : m_impl->height;
    m_impl->spriteManager.registerSprite(id, x, y, width, height, aw, ah);
}

void GuiSpriteAtlas::registerSprites(const std::vector<GuiSprite>& sprites)
{
    m_impl->spriteManager.registerSprites(sprites);
}

const GuiSprite* GuiSpriteAtlas::getSprite(const std::string& id) const
{
    return m_impl->spriteManager.getSprite(id);
}

bool GuiSpriteAtlas::hasSprite(const std::string& id) const
{
    return m_impl->spriteManager.hasSprite(id);
}

void GuiSpriteAtlas::clearSprites()
{
    m_impl->spriteManager.clearSprites();
}

size_t GuiSpriteAtlas::spriteCount() const
{
    return m_impl->spriteManager.spriteCount();
}

void GuiSpriteAtlas::setAtlasSize(i32 width, i32 height)
{
    m_impl->spriteManager.setAtlasSize(width, height);
}

i32 GuiSpriteAtlas::atlasWidth() const
{
    return m_impl->spriteManager.atlasWidth();
}

i32 GuiSpriteAtlas::atlasHeight() const
{
    return m_impl->spriteManager.atlasHeight();
}

// ============================================================================
// Vulkan资源访问
// ============================================================================

VkImageView GuiSpriteAtlas::imageView() const
{
    return m_impl->imageView;
}

VkSampler GuiSpriteAtlas::sampler() const
{
    return m_impl->sampler;
}

// ============================================================================
// TextureImage创建
// ============================================================================

ui::kagero::paint::TextureImage GuiSpriteAtlas::createTextureImage(const std::string& spriteId) const
{
    const GuiSprite* sprite = getSprite(spriteId);
    if (!sprite) {
        // 返回无效的TextureImage
        return ui::kagero::paint::TextureImage(VK_NULL_HANDLE,
            VK_NULL_HANDLE,
            0,
            0,
            0,
            0,
            0,
            0,
            m_atlasSlot,
            spriteId,
            ui::kagero::paint::ImageFormat::RGBA8);
    }

    return ui::kagero::paint::TextureImage(m_impl->imageView,
        m_impl->sampler,
        sprite->width,
        sprite->height,
        sprite->u0,
        sprite->v0,
        sprite->u1,
        sprite->v1,
        m_atlasSlot,
        spriteId,
        ui::kagero::paint::ImageFormat::RGBA8);
}

ui::kagero::paint::TextureImage GuiSpriteAtlas::createTextureImage(
    const std::string& spriteId, i32 customWidth, i32 customHeight) const
{
    const GuiSprite* sprite = getSprite(spriteId);
    if (!sprite) {
        // 返回无效的TextureImage
        return ui::kagero::paint::TextureImage(VK_NULL_HANDLE,
            VK_NULL_HANDLE,
            0,
            0,
            0,
            0,
            0,
            0,
            m_atlasSlot,
            spriteId,
            ui::kagero::paint::ImageFormat::RGBA8);
    }

    return ui::kagero::paint::TextureImage(m_impl->imageView,
        m_impl->sampler,
        customWidth,
        customHeight,
        sprite->u0,
        sprite->v0,
        sprite->u1,
        sprite->v1,
        m_atlasSlot,
        spriteId,
        ui::kagero::paint::ImageFormat::RGBA8);
}

} // namespace mc::client::renderer::trident::gui
