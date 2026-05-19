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

#include "GuiTextureManager.hpp"
#include "../util/VulkanUtils.hpp"
#include "GuiRenderer.hpp"
#include "client/resource/ResourceManager.hpp"
#include "common/resource/FolderResourcePack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <cstring>
#include <vector>
#include <spdlog/spdlog.h>
#include <stb_image.h>

namespace mc::client::renderer::trident::gui {

// ============================================================================
// 颜色常量（程序生成默认纹理用）
// ============================================================================

namespace GuiColors {
// 容器背景颜色
constexpr u32 CONTAINER_BG = 0xFFC6C6C6;     // 浅灰背景
constexpr u32 CONTAINER_BORDER = 0xFF555555; // 深灰边框

// 槽位颜色
constexpr u32 SLOT_BG = 0xFF8B8B8B;     // 槽位背景
constexpr u32 SLOT_BORDER = 0xFF373737; // 槽位边框

// 默认颜色
constexpr u32 DEFAULT_BG = 0xFF404040; // 默认背景
} // namespace GuiColors

// ============================================================================
// 构造函数 / 析构函数
// ============================================================================

GuiTextureManager::GuiTextureManager() = default;

GuiTextureManager::~GuiTextureManager()
{
    destroy();
}

// ============================================================================
// 初始化
// ============================================================================

Result<void> GuiTextureManager::initialize(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    ResourceManager* resourceManager)
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
    m_resourceManager = resourceManager;

    m_initialized = true;
    return {};
}

void GuiTextureManager::destroy()
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

    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_graphicsQueue = VK_NULL_HANDLE;
    m_resourceManager = nullptr;
    m_initialized = false;
    m_inventoryLoaded = false;
    m_craftingTableLoaded = false;
}

// ============================================================================
// 纹理加载
// ============================================================================

Result<void> GuiTextureManager::loadInventoryTexture()
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "GuiTextureManager not initialized");
    }

    // 尝试从资源管理器加载
    if (m_resourceManager != nullptr) {
        ResourceLocation location("minecraft:textures/gui/container/inventory");
        auto result = m_resourceManager->loadTextureRGBA(location);

        if (result.success()) {
            const auto& texture = result.value();
            spdlog::info("Loaded inventory texture: {}x{}", texture.width, texture.height);

            m_width = texture.width;
            m_height = texture.height;

            // 创建图像和上传数据
            auto imageResult = createImage(m_width, m_height);
            if (imageResult.failed()) {
                spdlog::warn("Failed to create image for inventory texture, using default");
                return createDefaultTextures();
            }

            auto viewResult = createImageView();
            if (viewResult.failed()) {
                spdlog::warn("Failed to create image view for inventory texture, using default");
                return createDefaultTextures();
            }

            auto samplerResult = createSampler();
            if (samplerResult.failed()) {
                spdlog::warn("Failed to create sampler for inventory texture, using default");
                return createDefaultTextures();
            }

            auto uploadResult = uploadTextureData(texture.pixels);
            if (uploadResult.failed()) {
                spdlog::warn("Failed to upload inventory texture, using default");
                return createDefaultTextures();
            }

            m_inventoryLoaded = true;
            spdlog::info("Inventory texture loaded successfully");
            return {};
        }
    }

    // 使用默认纹理
    spdlog::info("Inventory texture not found, using default generated texture");
    return createDefaultTextures();
}

Result<void> GuiTextureManager::loadCraftingTableTexture()
{
    // 暂时使用与背包相同的纹理
    // TODO: 后续加载 crafting_table.png
    m_craftingTableLoaded = m_inventoryLoaded;
    return {};
}

// ============================================================================
// 注册到渲染器
// ============================================================================

Result<u32> GuiTextureManager::registerToRenderer(GuiRenderer& renderer)
{
    if (!m_initialized || m_imageView == VK_NULL_HANDLE || m_sampler == VK_NULL_HANDLE) {
        return Error(ErrorCode::NotInitialized, "Texture not loaded");
    }

    auto result = renderer.registerAtlas("gui_container", m_imageView, m_sampler);
    if (result.failed()) {
        return result.error();
    }

    m_atlasSlot = static_cast<u8>(result.value());
    spdlog::info("Registered GUI container texture at slot {}", m_atlasSlot);
    return result;
}

// ============================================================================
// 绘制方法
// ============================================================================

void GuiTextureManager::drawInventoryBackground(GuiRenderer& gui, f64 x, f64 y)
{
    if (m_atlasSlot == 255) {
        // 未注册到渲染器，使用默认颜色绘制
        gui.fillRect(x,
            y,
            static_cast<f64>(ContainerTex::INVENTORY_BG_WIDTH),
            static_cast<f64>(ContainerTex::INVENTORY_BG_HEIGHT),
            GuiColors::CONTAINER_BG);
        gui.drawRect(x,
            y,
            static_cast<f64>(ContainerTex::INVENTORY_BG_WIDTH),
            static_cast<f64>(ContainerTex::INVENTORY_BG_HEIGHT),
            GuiColors::CONTAINER_BORDER);
        return;
    }

    // 使用纹理绘制
    gui.drawTexturedRect(x,
        y,
        static_cast<f64>(ContainerTex::INVENTORY_BG_WIDTH),
        static_cast<f64>(ContainerTex::INVENTORY_BG_HEIGHT),
        ContainerTex::INVENTORY_BG_U0,
        ContainerTex::INVENTORY_BG_V0,
        ContainerTex::INVENTORY_BG_U1,
        ContainerTex::INVENTORY_BG_V1,
        0xFFFFFFFF, // 白色色调
        m_atlasSlot);
}

void GuiTextureManager::drawCraftingTableBackground(GuiRenderer& gui, f64 x, f64 y)
{
    // 暂时使用背包纹理（纹理坐标相同）
    drawInventoryBackground(gui, x, y);
}

// ============================================================================
// 默认纹理创建
// ============================================================================

Result<void> GuiTextureManager::createDefaultTextures()
{
    // 创建一个简单的默认容器背景纹理
    constexpr i32 DEFAULT_WIDTH = 256;
    constexpr i32 DEFAULT_HEIGHT = 256;

    m_width = DEFAULT_WIDTH;
    m_height = DEFAULT_HEIGHT;

    std::vector<u8> data(DEFAULT_WIDTH * DEFAULT_HEIGHT * 4, 0);

    // 填充默认背景
    for (i32 y = 0; y < DEFAULT_HEIGHT; ++y) {
        for (i32 x = 0; x < DEFAULT_WIDTH; ++x) {
            const i32 idx = (y * DEFAULT_WIDTH + x) * 4;

            // 背包屏幕区域 (0, 0) - (176, 166)
            if (x < ContainerTex::INVENTORY_BG_WIDTH && y < ContainerTex::INVENTORY_BG_HEIGHT) {
                bool isBorder = (x == 0 || x == ContainerTex::INVENTORY_BG_WIDTH - 1 || y == 0 ||
                    y == ContainerTex::INVENTORY_BG_HEIGHT - 1);

                u32 color = isBorder ? GuiColors::CONTAINER_BORDER : GuiColors::CONTAINER_BG;
                data[idx + 0] = (color >> 0) & 0xFF;  // R
                data[idx + 1] = (color >> 8) & 0xFF;  // G
                data[idx + 2] = (color >> 16) & 0xFF; // B
                data[idx + 3] = 0xFF;                 // A
            } else {
                // 其他区域透明
                data[idx + 3] = 0x00;
            }
        }
    }

    // 创建图像
    auto imageResult = createImage(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    if (imageResult.failed()) {
        return imageResult;
    }

    auto viewResult = createImageView();
    if (viewResult.failed()) {
        return viewResult;
    }

    auto samplerResult = createSampler();
    if (samplerResult.failed()) {
        return samplerResult;
    }

    auto uploadResult = uploadTextureData(data);
    if (uploadResult.failed()) {
        return uploadResult;
    }

    m_inventoryLoaded = true;
    m_craftingTableLoaded = true;
    return {};
}

// ============================================================================
// Vulkan 辅助方法
// ============================================================================

Result<void> GuiTextureManager::createImage(u32 width, u32 height)
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
        return Error(ErrorCode::OutOfMemory, "Failed to create GUI texture image");
    }

    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, m_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    auto memTypeResult = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memTypeResult.failed()) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return memTypeResult.error();
    }
    allocInfo.memoryTypeIndex = memTypeResult.value();

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS) {
        vkDestroyImage(m_device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
        return Error(ErrorCode::OutOfMemory, "Failed to allocate GUI texture memory");
    }

    vkBindImageMemory(m_device, m_image, m_imageMemory, 0);
    return {};
}

Result<void> GuiTextureManager::createImageView()
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
        return Error(ErrorCode::InitializationFailed, "Failed to create GUI texture image view");
    }

    return {};
}

Result<void> GuiTextureManager::createSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST; // GUI使用最近邻过滤
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
        return Error(ErrorCode::InitializationFailed, "Failed to create GUI texture sampler");
    }

    return {};
}

Result<void> GuiTextureManager::uploadTextureData(const std::vector<u8>& data)
{
    const VkDeviceSize imageSize = data.size();

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
    auto memTypeResult = findMemoryType(
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
    VkCommandBuffer cmd = beginSingleTimeCommands();

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

    endSingleTimeCommands(cmd);

    // 清理暂存缓冲区
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingMemory, nullptr);

    return {};
}

Result<u32> GuiTextureManager::findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    return VulkanUtils::findMemoryType(m_physicalDevice, typeFilter, properties);
}

VkCommandBuffer GuiTextureManager::beginSingleTimeCommands()
{
    return VulkanUtils::beginSingleTimeCommands(m_device, m_commandPool);
}

void GuiTextureManager::endSingleTimeCommands(VkCommandBuffer cmd)
{
    VulkanUtils::endSingleTimeCommands(m_device, m_commandPool, m_graphicsQueue, cmd);
}

} // namespace mc::client::renderer::trident::gui
