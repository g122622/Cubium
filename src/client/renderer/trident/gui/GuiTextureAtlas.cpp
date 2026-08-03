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

#include "GuiTextureAtlas.hpp"
#include "GuiRenderer.hpp"
#include "client/renderer/trident/gui/GuiSprite.hpp"
#include "client/renderer/trident/util/VulkanUtils.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <cstring>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident::gui {

// ============================================================================
// 颜色常量
// ============================================================================

namespace GuiColors {
// 槽位颜色
constexpr u32 SLOT_BG = 0xFF8B8B8B;     // 槽位背景灰色
constexpr u32 SLOT_BORDER = 0xFF373737; // 槽位边框深灰
constexpr u32 SLOT_INNER = 0xFFC6C6C6;  // 槽位内部浅灰

// 容器背景颜色
constexpr u32 CONTAINER_BG = 0xFFC6C6C6;     // 容器背景
constexpr u32 CONTAINER_BORDER = 0xFF555555; // 边框

// 默认颜色
constexpr u32 DEFAULT_BG = 0xFF404040; // 默认背景
} // namespace GuiColors

// ============================================================================
// 构造函数 / 析构函数
// ============================================================================

GuiTextureAtlas::GuiTextureAtlas() = default;

GuiTextureAtlas::~GuiTextureAtlas()
{
    destroy();
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> GuiTextureAtlas::initialize(
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

    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;

    // 创建默认纹理
    auto result = _createDefaultTextures();
    if (result.failed()) {
        return result;
    }

    m_initialized = true;
    return {};
}

void GuiTextureAtlas::destroy()
{
    if (!m_initialized) {
        return;
    }

    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

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

    m_regions.clear();
    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_graphicsQueue = VK_NULL_HANDLE;
    m_initialized = false;
}

// ============================================================================
// 默认纹理创建
// ============================================================================

Result<void> GuiTextureAtlas::_createDefaultTextures()
{
    // 创建一个包含默认GUI纹理的小图集
    // 图集布局：
    // - (0,0) 到 (18,18): 槽位背景
    // - (20,0) 到 (196,166): 容器背景

    constexpr i32 ATLAS_WIDTH = DEFAULT_ATLAS_WIDTH;
    constexpr i32 ATLAS_HEIGHT = DEFAULT_ATLAS_HEIGHT;

    m_width = ATLAS_WIDTH;
    m_height = ATLAS_HEIGHT;

    std::vector<u8> atlasData(ATLAS_WIDTH * ATLAS_HEIGHT * 4, 0);

    // 创建槽位背景纹理
    _createSlotBackground(atlasData.data(), ATLAS_WIDTH, ATLAS_HEIGHT);

    // 创建容器背景纹理
    _createContainerBackground(atlasData.data(), ATLAS_WIDTH, ATLAS_HEIGHT);

    // 注册纹理区域
    // 槽位背景
    GuiTextureRegion slotRegion;
    slotRegion.u0 = 0.0f / ATLAS_WIDTH;
    slotRegion.v0 = 0.0f / ATLAS_HEIGHT;
    slotRegion.u1 = static_cast<f64>(DEFAULT_SLOT_SIZE) / ATLAS_WIDTH;
    slotRegion.v1 = static_cast<f64>(DEFAULT_SLOT_SIZE) / ATLAS_HEIGHT;
    slotRegion.width = DEFAULT_SLOT_SIZE;
    slotRegion.height = DEFAULT_SLOT_SIZE;
    m_regions["minecraft:gui/slot"] = slotRegion;

    // 容器背景
    GuiTextureRegion containerRegion;
    containerRegion.u0 = 20.0f / ATLAS_WIDTH;
    containerRegion.v0 = 0.0f / ATLAS_HEIGHT;
    containerRegion.u1 = static_cast<f64>(20 + DEFAULT_CONTAINER_WIDTH) / ATLAS_WIDTH;
    containerRegion.v1 = static_cast<f64>(DEFAULT_CONTAINER_HEIGHT) / ATLAS_HEIGHT;
    containerRegion.width = DEFAULT_CONTAINER_WIDTH;
    containerRegion.height = DEFAULT_CONTAINER_HEIGHT;
    m_regions["minecraft:gui/container/generic"] = containerRegion;

    // 工作台背景
    m_regions["minecraft:gui/container/crafting_table"] = containerRegion;

    // 玩家背包背景
    m_regions["minecraft:gui/container/inventory"] = containerRegion;

    // 创建图像
    auto imageResult = _createImage(ATLAS_WIDTH, ATLAS_HEIGHT);
    if (imageResult.failed()) {
        return imageResult;
    }

    // 创建图像视图
    auto viewResult = _createImageView();
    if (viewResult.failed()) {
        return viewResult;
    }

    // 创建采样器
    auto samplerResult = _createSampler();
    if (samplerResult.failed()) {
        return samplerResult;
    }

    // 上传纹理数据
    return _uploadTextureData(atlasData);
}

void GuiTextureAtlas::_createSlotBackground(u8* data, i32 width, i32 height)
{
    // 槽位背景尺寸：18x18像素
    // 边框：1像素深灰，内部：浅灰

    for (i32 y = 0; y < DEFAULT_SLOT_SIZE && y < height; ++y) {
        for (i32 x = 0; x < DEFAULT_SLOT_SIZE && x < width; ++x) {
            i32 idx = (y * width + x) * 4;

            bool isBorder = (x == 0 || x == DEFAULT_SLOT_SIZE - 1 || y == 0 || y == DEFAULT_SLOT_SIZE - 1);

            if (isBorder) {
                // 边框颜色
                data[idx + 0] = (GuiColors::SLOT_BORDER >> 0) & 0xFF;  // R
                data[idx + 1] = (GuiColors::SLOT_BORDER >> 8) & 0xFF;  // G
                data[idx + 2] = (GuiColors::SLOT_BORDER >> 16) & 0xFF; // B
                data[idx + 3] = 0xFF;                                  // A
            } else {
                // 内部颜色
                data[idx + 0] = (GuiColors::SLOT_BG >> 0) & 0xFF;
                data[idx + 1] = (GuiColors::SLOT_BG >> 8) & 0xFF;
                data[idx + 2] = (GuiColors::SLOT_BG >> 16) & 0xFF;
                data[idx + 3] = 0xFF;
            }
        }
    }
}

void GuiTextureAtlas::_createContainerBackground(u8* data, i32 width, i32 height)
{
    // 容器背景尺寸：176x166像素
    // 起始位置：(20, 0)

    constexpr i32 OFFSET_X = 20;
    constexpr i32 OFFSET_Y = 0;

    for (i32 y = 0; y < DEFAULT_CONTAINER_HEIGHT && (OFFSET_Y + y) < height; ++y) {
        for (i32 x = 0; x < DEFAULT_CONTAINER_WIDTH && (OFFSET_X + x) < width; ++x) {
            i32 idx = ((OFFSET_Y + y) * width + (OFFSET_X + x)) * 4;

            bool isBorder = (x == 0 || x == DEFAULT_CONTAINER_WIDTH - 1 || y == 0 || y == DEFAULT_CONTAINER_HEIGHT - 1);

            if (isBorder) {
                // 边框颜色
                data[idx + 0] = (GuiColors::CONTAINER_BORDER >> 0) & 0xFF;
                data[idx + 1] = (GuiColors::CONTAINER_BORDER >> 8) & 0xFF;
                data[idx + 2] = (GuiColors::CONTAINER_BORDER >> 16) & 0xFF;
                data[idx + 3] = 0xFF;
            } else {
                // 背景颜色
                data[idx + 0] = (GuiColors::CONTAINER_BG >> 0) & 0xFF;
                data[idx + 1] = (GuiColors::CONTAINER_BG >> 8) & 0xFF;
                data[idx + 2] = (GuiColors::CONTAINER_BG >> 16) & 0xFF;
                data[idx + 3] = 0xFF;
            }
        }
    }
}

// ============================================================================
// 纹理加载
// ============================================================================

Result<void> GuiTextureAtlas::loadGuiTexture(const ResourceLocation& location)
{
    // TODO: 从资源包加载GUI纹理
    // 暂时使用默认纹理
    (void)location;
    return {};
}

Result<void> GuiTextureAtlas::loadDefaultTextures()
{
    // 默认纹理已在 createDefaultTextures 中创建
    return {};
}

// ============================================================================
// 渲染
// ============================================================================

void GuiTextureAtlas::drawTexture(GuiRenderer& gui, const std::string& textureId, f64 x, f64 y, f64 width, f64 height)
{
    const GuiTextureRegion* region = getRegion(textureId);
    if (region == nullptr) {
        // 使用默认颜色绘制
        gui.fillRect(x, y, width, height, GuiColors::DEFAULT_BG);
        return;
    }

    // TODO: 使用纹理渲染
    // 暂时使用默认颜色绘制
    gui.fillRect(x, y, width, height, GuiColors::CONTAINER_BG);
    gui.drawRect(x, y, width, height, GuiColors::CONTAINER_BORDER);
}

void GuiTextureAtlas::drawTextureRegion(GuiRenderer& gui,
    const std::string& textureId,
    f64 x,
    f64 y,
    i32 regionX,
    i32 regionY,
    i32 regionWidth,
    i32 regionHeight,
    f64 width,
    f64 height)
{
    // TODO: 实现纹理区域渲染，当前忽略region参数
    (void)regionX;
    (void)regionY;
    (void)regionWidth;
    (void)regionHeight;

    const GuiTextureRegion* region = getRegion(textureId);
    if (region == nullptr) {
        gui.fillRect(x, y, width, height, GuiColors::DEFAULT_BG);
        return;
    }

    // TODO: 使用纹理区域渲染
    gui.fillRect(x, y, width, height, GuiColors::CONTAINER_BG);
}

// ============================================================================
// 查询
// ============================================================================

bool GuiTextureAtlas::hasTexture(const std::string& textureId) const
{
    return m_regions.find(textureId) != m_regions.end();
}

const GuiTextureRegion* GuiTextureAtlas::getRegion(const std::string& textureId) const
{
    auto it = m_regions.find(textureId);
    if (it != m_regions.end()) {
        return &it->second;
    }
    return nullptr;
}

// ============================================================================
// 精灵管理
// ============================================================================

void GuiTextureAtlas::registerSprite(const GuiSprite& sprite)
{
    if (sprite.id.empty()) {
        return;
    }
    m_sprites[sprite.id] = sprite;
}

void GuiTextureAtlas::registerSprite(
    const std::string& id, i32 x, i32 y, i32 width, i32 height, i32 atlasWidth, i32 atlasHeight)
{
    GuiSprite sprite(id, x, y, width, height, atlasWidth, atlasHeight);
    m_sprites[id] = sprite;
}

void GuiTextureAtlas::registerSprites(const std::vector<GuiSprite>& sprites)
{
    for (const auto& sprite : sprites) {
        registerSprite(sprite);
    }
}

const GuiSprite* GuiTextureAtlas::getSprite(const std::string& id) const
{
    auto it = m_sprites.find(id);
    if (it != m_sprites.end()) {
        return &it->second;
    }
    return nullptr;
}

bool GuiTextureAtlas::hasSprite(const std::string& id) const
{
    return m_sprites.find(id) != m_sprites.end();
}

void GuiTextureAtlas::clearSprites()
{
    m_sprites.clear();
}

void GuiTextureAtlas::setAtlasSize(i32 width, i32 height)
{
    m_atlasWidth = width;
    m_atlasHeight = height;
}

// ============================================================================
// Vulkan 辅助方法
// ============================================================================

Result<void> GuiTextureAtlas::_createImage(u32 width, u32 height)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create GUI texture atlas image");
    }

    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, m_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    auto memTypeResult = _findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memTypeResult.failed()) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return memTypeResult.error();
    }
    allocInfo.memoryTypeIndex = memTypeResult.value();

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return Error(ErrorCode::OutOfMemory, "Failed to allocate GUI texture atlas memory");
    }

    vkBindImageMemory(m_device, m_image, m_imageMemory, 0);
    return {};
}

Result<void> GuiTextureAtlas::_createImageView()
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create GUI texture atlas image view");
    }

    return {};
}

Result<void> GuiTextureAtlas::_createSampler()
{
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

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create GUI texture atlas sampler");
    }

    return {};
}

Result<void> GuiTextureAtlas::_uploadTextureData(const std::vector<u8>& data)
{
    VkDeviceSize imageSize = data.size();

    // 创建暂存缓冲区
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create staging buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, stagingBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    auto memTypeResult = _findMemoryType(
        memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (memTypeResult.failed()) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        return memTypeResult.error();
    }
    allocInfo.memoryTypeIndex = memTypeResult.value();

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        return Error(ErrorCode::OutOfMemory, "Failed to allocate staging memory");
    }

    vkBindBufferMemory(m_device, stagingBuffer, stagingMemory, 0);

    // 复制数据
    void* mappedData = nullptr;
    if (vkMapMemory(m_device, stagingMemory, 0, imageSize, 0, &mappedData) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingMemory, nullptr);
        return Error(ErrorCode::OperationFailed, "Failed to map staging memory");
    }
    std::memcpy(mappedData, data.data(), data.size());
    vkUnmapMemory(m_device, stagingMemory);

    // 转换图像布局并复制
    VkCommandBuffer cmd = _beginSingleTimeCommands();

    // 转换到传输目标布局
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
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
    region.imageExtent = {m_width, m_height, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // 转换到着色器只读布局
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

    _endSingleTimeCommands(cmd);

    // 清理暂存缓冲区
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingMemory, nullptr);

    return {};
}

Result<u32> GuiTextureAtlas::_findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    return mc::client::renderer::VulkanUtils::findMemoryType(m_physicalDevice, typeFilter, properties);
}

VkCommandBuffer GuiTextureAtlas::_beginSingleTimeCommands()
{
    return mc::client::renderer::VulkanUtils::beginSingleTimeCommands(m_device, m_commandPool);
}

void GuiTextureAtlas::_endSingleTimeCommands(VkCommandBuffer cmd)
{
    // 使用 fence 版本，避免阻塞整个 GPU 队列
    mc::client::renderer::VulkanUtils::endSingleTimeCommands(m_device, m_commandPool, m_graphicsQueue, cmd);
}

} // namespace mc::client::renderer::trident::gui