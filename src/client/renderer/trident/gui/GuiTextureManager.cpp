/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
#include "GuiRenderer.hpp"
#include "client/renderer/trident/util/VulkanUtils.hpp"
#include "client/resource/ResourceManager.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident::gui {

// 颜色常量已移至 GuiTextureManager.hpp（GuiColors 命名空间）
// 仅保留 cpp 内部使用的默认纹理颜色常量
namespace {
constexpr u32 FURNACE_FIRE_BG = 0xFF8B8B8B;  // 火焰占位颜色（默认纹理用）
constexpr u32 FURNACE_ARROW_BG = 0xFF8B8B8B; // 箭头占位颜色（默认纹理用）
} // namespace

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

    _destroyEntry(m_furnaceEntry);
    _destroyEntry(m_craftingTableEntry);
    _destroyEntry(m_inventoryEntry);

    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_graphicsQueue = VK_NULL_HANDLE;
    m_resourceManager = nullptr;
    m_initialized = false;
}

void GuiTextureManager::_destroyEntry(ContainerTextureEntry& entry)
{
    if (entry.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, entry.sampler, nullptr);
        entry.sampler = VK_NULL_HANDLE;
    }

    if (entry.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, entry.imageView, nullptr);
        entry.imageView = VK_NULL_HANDLE;
    }

    if (entry.image != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, entry.image, nullptr);
        entry.image = VK_NULL_HANDLE;
    }

    if (entry.imageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, entry.imageMemory, nullptr);
        entry.imageMemory = VK_NULL_HANDLE;
    }

    entry.atlasSlot = 255;
    entry.loaded = false;
}

// ============================================================================
// 纹理加载
// ============================================================================

Result<void> GuiTextureManager::loadInventoryTexture()
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "GuiTextureManager not initialized");
    }

    return _loadTexture("minecraft:textures/gui/container/inventory", m_inventoryEntry, true);
}

Result<void> GuiTextureManager::loadCraftingTableTexture()
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "GuiTextureManager not initialized");
    }

    auto result = _loadTexture("minecraft:textures/gui/container/crafting_table", m_craftingTableEntry, false);
    if (result.failed()) {
        // 工作台纹理加载失败时，复用背包纹理
        spdlog::info("Crafting table texture not available, falling back to inventory texture");
        m_craftingTableEntry.loaded = m_inventoryEntry.loaded;
        m_craftingTableEntry.atlasSlot = m_inventoryEntry.atlasSlot;
        m_craftingTableEntry.imageView = m_inventoryEntry.imageView;
        m_craftingTableEntry.sampler = m_inventoryEntry.sampler;
        m_craftingTableEntry.width = m_inventoryEntry.width;
        m_craftingTableEntry.height = m_inventoryEntry.height;
        // 注意：复用背包纹理时不拥有Vulkan资源，所以不设置image和imageMemory
        return {};
    }
    return {};
}

Result<void> GuiTextureManager::loadFurnaceTexture()
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "GuiTextureManager not initialized");
    }

    return _loadTexture("minecraft:textures/gui/container/furnace", m_furnaceEntry, true);
}

Result<void> GuiTextureManager::_loadTexture(
    const std::string& resourcePath, ContainerTextureEntry& entry, bool createDefault)
{
    // 尝试从资源管理器加载
    if (m_resourceManager != nullptr) {
        ResourceLocation location(resourcePath);
        auto result = m_resourceManager->loadTextureRGBA(location);

        if (result.success()) {
            const auto& texture = result.value();
            spdlog::info("Loaded container texture '{}': {}x{}", resourcePath, texture.width, texture.height);

            entry.width = texture.width;
            entry.height = texture.height;

            // 创建图像和上传数据
            auto imageResult = _createImage(entry.width, entry.height, entry);
            if (imageResult.failed()) {
                spdlog::warn("Failed to create image for texture '{}', using default", resourcePath);
                if (createDefault) {
                    if (resourcePath.find("furnace") != std::string::npos) {
                        return _createDefaultFurnaceTexture(entry);
                    }
                    return _createDefaultContainerTexture(entry);
                }
                return imageResult;
            }

            auto viewResult = _createImageView(entry);
            if (viewResult.failed()) {
                spdlog::warn("Failed to create image view for texture '{}', using default", resourcePath);
                if (createDefault) {
                    if (resourcePath.find("furnace") != std::string::npos) {
                        return _createDefaultFurnaceTexture(entry);
                    }
                    return _createDefaultContainerTexture(entry);
                }
                return viewResult;
            }

            auto samplerResult = _createSampler(entry);
            if (samplerResult.failed()) {
                spdlog::warn("Failed to create sampler for texture '{}', using default", resourcePath);
                if (createDefault) {
                    if (resourcePath.find("furnace") != std::string::npos) {
                        return _createDefaultFurnaceTexture(entry);
                    }
                    return _createDefaultContainerTexture(entry);
                }
                return samplerResult;
            }

            auto uploadResult = _uploadTextureData(texture.pixels, entry);
            if (uploadResult.failed()) {
                spdlog::warn("Failed to upload texture '{}', using default", resourcePath);
                if (createDefault) {
                    if (resourcePath.find("furnace") != std::string::npos) {
                        return _createDefaultFurnaceTexture(entry);
                    }
                    return _createDefaultContainerTexture(entry);
                }
                return uploadResult;
            }

            entry.loaded = true;
            spdlog::info("Container texture '{}' loaded successfully", resourcePath);
            return {};
        }
    }

    // 使用默认纹理
    spdlog::info("Container texture '{}' not found, using default generated texture", resourcePath);
    if (!createDefault) {
        return Error(ErrorCode::ResourceNotFound, "Texture not found: " + resourcePath);
    }
    if (resourcePath.find("furnace") != std::string::npos) {
        return _createDefaultFurnaceTexture(entry);
    }
    return _createDefaultContainerTexture(entry);
}

// ============================================================================
// 注册到渲染器
// ============================================================================

Result<u32> GuiTextureManager::registerToRenderer(GuiRenderer& renderer)
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "GuiTextureManager not initialized");
    }

    u32 registeredCount = 0;

    // 注册背包纹理
    if (m_inventoryEntry.loaded && m_inventoryEntry.imageView != VK_NULL_HANDLE &&
        m_inventoryEntry.sampler != VK_NULL_HANDLE) {
        auto result = renderer.registerAtlas("gui_container", m_inventoryEntry.imageView, m_inventoryEntry.sampler);
        if (result.success()) {
            m_inventoryEntry.atlasSlot = static_cast<u8>(result.value());
            spdlog::info("Registered inventory texture at atlas slot {}", m_inventoryEntry.atlasSlot);
            ++registeredCount;
        } else {
            spdlog::warn("Failed to register inventory texture: {}", result.error().toString());
        }
    }

    // 注册工作台纹理（如果有独立的Vulkan资源）
    if (m_craftingTableEntry.loaded && m_craftingTableEntry.imageView != VK_NULL_HANDLE &&
        m_craftingTableEntry.sampler != VK_NULL_HANDLE &&
        m_craftingTableEntry.imageView != m_inventoryEntry.imageView) {
        auto result =
            renderer.registerAtlas("gui_crafting_table", m_craftingTableEntry.imageView, m_craftingTableEntry.sampler);
        if (result.success()) {
            m_craftingTableEntry.atlasSlot = static_cast<u8>(result.value());
            spdlog::info("Registered crafting table texture at atlas slot {}", m_craftingTableEntry.atlasSlot);
            ++registeredCount;
        } else {
            spdlog::warn("Failed to register crafting table texture: {}", result.error().toString());
        }
    } else if (m_craftingTableEntry.loaded) {
        // 复用背包纹理的图集槽位
        m_craftingTableEntry.atlasSlot = m_inventoryEntry.atlasSlot;
    }

    // 注册熔炉纹理
    if (m_furnaceEntry.loaded && m_furnaceEntry.imageView != VK_NULL_HANDLE &&
        m_furnaceEntry.sampler != VK_NULL_HANDLE) {
        auto result = renderer.registerAtlas("gui_furnace", m_furnaceEntry.imageView, m_furnaceEntry.sampler);
        if (result.success()) {
            m_furnaceEntry.atlasSlot = static_cast<u8>(result.value());
            spdlog::info("Registered furnace texture at atlas slot {}", m_furnaceEntry.atlasSlot);
            ++registeredCount;
        } else {
            spdlog::warn("Failed to register furnace texture: {}", result.error().toString());
        }
    }

    return registeredCount;
}

// ============================================================================
// 绘制方法
// ============================================================================

void GuiTextureManager::drawInventoryBackground(GuiRenderer& gui, f64 x, f64 y)
{
    if (m_inventoryEntry.atlasSlot == 255) {
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
        m_inventoryEntry.atlasSlot);
}

void GuiTextureManager::drawCraftingTableBackground(GuiRenderer& gui, f64 x, f64 y)
{
    if (m_craftingTableEntry.atlasSlot == 255) {
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

    // 使用纹理绘制（当前复用背包纹理的UV坐标）
    gui.drawTexturedRect(x,
        y,
        static_cast<f64>(ContainerTex::CRAFTING_TABLE_BG_WIDTH),
        static_cast<f64>(ContainerTex::CRAFTING_TABLE_BG_HEIGHT),
        ContainerTex::CRAFTING_TABLE_BG_U0,
        ContainerTex::CRAFTING_TABLE_BG_V0,
        ContainerTex::CRAFTING_TABLE_BG_U1,
        ContainerTex::CRAFTING_TABLE_BG_V1,
        0xFFFFFFFF,
        m_craftingTableEntry.atlasSlot);
}

void GuiTextureManager::drawFurnaceBackground(GuiRenderer& gui, f64 x, f64 y)
{
    if (m_furnaceEntry.atlasSlot == 255) {
        // 未注册到渲染器，使用默认颜色绘制
        gui.fillRect(x,
            y,
            static_cast<f64>(ContainerTex::FURNACE_BG_WIDTH),
            static_cast<f64>(ContainerTex::FURNACE_BG_HEIGHT),
            GuiColors::CONTAINER_BG);
        gui.drawRect(x,
            y,
            static_cast<f64>(ContainerTex::FURNACE_BG_WIDTH),
            static_cast<f64>(ContainerTex::FURNACE_BG_HEIGHT),
            GuiColors::CONTAINER_BORDER);
        return;
    }

    // 使用纹理绘制熔炉背景
    gui.drawTexturedRect(x,
        y,
        static_cast<f64>(ContainerTex::FURNACE_BG_WIDTH),
        static_cast<f64>(ContainerTex::FURNACE_BG_HEIGHT),
        ContainerTex::FURNACE_BG_U0,
        ContainerTex::FURNACE_BG_V0,
        ContainerTex::FURNACE_BG_U1,
        ContainerTex::FURNACE_BG_V1,
        0xFFFFFFFF,
        m_furnaceEntry.atlasSlot);
}

void GuiTextureManager::drawFurnaceLitProgress(GuiRenderer& gui, f64 x, f64 y, f32 litProgress)
{
    if (m_furnaceEntry.atlasSlot == 255) {
        return;
    }

    // 计算可见的火焰高度（从底部向上填充）
    // 与 MC Java 的 AbstractFurnaceScreen.renderBg() 一致：
    // l = ceil(litProgress * 13.0) + 1，范围 1~14
    litProgress = std::clamp(litProgress, 0.0f, 1.0f);
    if (litProgress <= 0.0f) {
        return; // 不燃烧时不绘制火焰
    }
    const i32 visibleHeight = static_cast<i32>(std::ceil(static_cast<f64>(litProgress) * 13.0)) + 1;
    const i32 clampedHeight = std::clamp(visibleHeight, 1, ContainerTex::FURNACE_LIT_HEIGHT);

    // 计算UV坐标：从火焰图标的底部向上裁剪
    const f64 fullV0 = ContainerTex::FURNACE_LIT_V0;
    const f64 fullV1 = ContainerTex::FURNACE_LIT_V1;
    const f64 vRange = fullV1 - fullV0;
    const f64 visibleV0 = fullV1 - (static_cast<f64>(clampedHeight) / ContainerTex::FURNACE_LIT_HEIGHT) * vRange;

    // 屏幕位置：火焰底部对齐
    const f64 screenX = x + ContainerTex::FURNACE_LIT_SCREEN_X;
    const f64 screenY = y + ContainerTex::FURNACE_LIT_SCREEN_Y + (ContainerTex::FURNACE_LIT_HEIGHT - clampedHeight);

    gui.drawTexturedRect(screenX,
        screenY,
        static_cast<f64>(ContainerTex::FURNACE_LIT_WIDTH),
        static_cast<f64>(clampedHeight),
        ContainerTex::FURNACE_LIT_U0,
        visibleV0,
        ContainerTex::FURNACE_LIT_U1,
        fullV1,
        0xFFFFFFFF,
        m_furnaceEntry.atlasSlot);
}

void GuiTextureManager::drawFurnaceBurnProgress(GuiRenderer& gui, f64 x, f64 y, f32 burnProgress)
{
    if (m_furnaceEntry.atlasSlot == 255) {
        return;
    }

    // 计算可见的箭头宽度（从左向右填充）
    // 与 MC Java 的 AbstractFurnaceScreen.renderBg() 一致：
    // j1 = ceil(burnProgress * 24.0)，范围 0~24
    burnProgress = std::clamp(burnProgress, 0.0f, 1.0f);
    if (burnProgress <= 0.0f) {
        return; // 无进度时不绘制箭头
    }
    const i32 visibleWidth = static_cast<i32>(std::ceil(static_cast<f64>(burnProgress) * 24.0));
    const i32 clampedWidth = std::clamp(visibleWidth, 0, ContainerTex::FURNACE_ARROW_WIDTH);

    if (clampedWidth == 0) {
        return;
    }

    // 计算UV坐标：从箭头左侧向右裁剪
    const f64 fullU0 = ContainerTex::FURNACE_ARROW_U0;
    const f64 fullU1 = ContainerTex::FURNACE_ARROW_U1;
    const f64 uRange = fullU1 - fullU0;
    const f64 visibleU1 = fullU0 + (static_cast<f64>(clampedWidth) / ContainerTex::FURNACE_ARROW_WIDTH) * uRange;

    // 屏幕位置：箭头左对齐
    const f64 screenX = x + ContainerTex::FURNACE_ARROW_SCREEN_X;
    const f64 screenY = y + ContainerTex::FURNACE_ARROW_SCREEN_Y;

    gui.drawTexturedRect(screenX,
        screenY,
        static_cast<f64>(clampedWidth),
        static_cast<f64>(ContainerTex::FURNACE_ARROW_HEIGHT),
        fullU0,
        ContainerTex::FURNACE_ARROW_V0,
        visibleU1,
        ContainerTex::FURNACE_ARROW_V1,
        0xFFFFFFFF,
        m_furnaceEntry.atlasSlot);
}

// ============================================================================
// 默认纹理创建
// ============================================================================

Result<void> GuiTextureManager::_createDefaultContainerTexture(ContainerTextureEntry& entry)
{
    constexpr i32 DEFAULT_WIDTH = 256;
    constexpr i32 DEFAULT_HEIGHT = 256;

    entry.width = DEFAULT_WIDTH;
    entry.height = DEFAULT_HEIGHT;

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
    auto imageResult = _createImage(DEFAULT_WIDTH, DEFAULT_HEIGHT, entry);
    if (imageResult.failed()) {
        return imageResult;
    }

    auto viewResult = _createImageView(entry);
    if (viewResult.failed()) {
        return viewResult;
    }

    auto samplerResult = _createSampler(entry);
    if (samplerResult.failed()) {
        return samplerResult;
    }

    auto uploadResult = _uploadTextureData(data, entry);
    if (uploadResult.failed()) {
        return uploadResult;
    }

    entry.loaded = true;
    return {};
}

Result<void> GuiTextureManager::_createDefaultFurnaceTexture(ContainerTextureEntry& entry)
{
    constexpr i32 DEFAULT_WIDTH = 256;
    constexpr i32 DEFAULT_HEIGHT = 256;

    entry.width = DEFAULT_WIDTH;
    entry.height = DEFAULT_HEIGHT;

    std::vector<u8> data(DEFAULT_WIDTH * DEFAULT_HEIGHT * 4, 0);

    // 填充默认背景
    for (i32 y = 0; y < DEFAULT_HEIGHT; ++y) {
        for (i32 x = 0; x < DEFAULT_WIDTH; ++x) {
            const i32 idx = (y * DEFAULT_WIDTH + x) * 4;

            // 熔炉背景区域 (0, 0) - (176, 166)
            if (x < ContainerTex::FURNACE_BG_WIDTH && y < ContainerTex::FURNACE_BG_HEIGHT) {
                bool isBorder = (x == 0 || x == ContainerTex::FURNACE_BG_WIDTH - 1 || y == 0 ||
                    y == ContainerTex::FURNACE_BG_HEIGHT - 1);

                u32 color = isBorder ? GuiColors::CONTAINER_BORDER : GuiColors::CONTAINER_BG;
                data[idx + 0] = (color >> 0) & 0xFF;
                data[idx + 1] = (color >> 8) & 0xFF;
                data[idx + 2] = (color >> 16) & 0xFF;
                data[idx + 3] = 0xFF;
            }
            // 火焰指示器区域 (176, 0) - (190, 14)
            else if (x >= 176 && x < 190 && y >= 0 && y < 14) {
                u32 color = GuiColors::FURNACE_FIRE_FILL;
                data[idx + 0] = (color >> 0) & 0xFF;
                data[idx + 1] = (color >> 8) & 0xFF;
                data[idx + 2] = (color >> 16) & 0xFF;
                data[idx + 3] = 0xFF;
            }
            // 进度箭头区域 (176, 14) - (200, 30)
            else if (x >= 176 && x < 200 && y >= 14 && y < 30) {
                u32 color = GuiColors::FURNACE_ARROW_FILL;
                data[idx + 0] = (color >> 0) & 0xFF;
                data[idx + 1] = (color >> 8) & 0xFF;
                data[idx + 2] = (color >> 16) & 0xFF;
                data[idx + 3] = 0xFF;
            } else {
                // 其他区域透明
                data[idx + 3] = 0x00;
            }
        }
    }

    // 创建图像
    auto imageResult = _createImage(DEFAULT_WIDTH, DEFAULT_HEIGHT, entry);
    if (imageResult.failed()) {
        return imageResult;
    }

    auto viewResult = _createImageView(entry);
    if (viewResult.failed()) {
        return viewResult;
    }

    auto samplerResult = _createSampler(entry);
    if (samplerResult.failed()) {
        return samplerResult;
    }

    auto uploadResult = _uploadTextureData(data, entry);
    if (uploadResult.failed()) {
        return uploadResult;
    }

    entry.loaded = true;
    return {};
}

// ============================================================================
// Vulkan 辅助方法
// ============================================================================

Result<void> GuiTextureManager::_createImage(u32 width, u32 height, ContainerTextureEntry& entry)
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

    if (vkCreateImage(m_device, &imageInfo, nullptr, &entry.image) != VK_SUCCESS) {
        return Error(ErrorCode::OutOfMemory, "Failed to create GUI texture image");
    }

    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, entry.image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    auto memTypeResult = _findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memTypeResult.failed()) {
        vkDestroyImage(m_device, entry.image, nullptr);
        entry.image = VK_NULL_HANDLE;
        return memTypeResult.error();
    }
    allocInfo.memoryTypeIndex = memTypeResult.value();

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &entry.imageMemory) != VK_SUCCESS) {
        vkDestroyImage(m_device, entry.image, nullptr);
        entry.image = VK_NULL_HANDLE;
        return Error(ErrorCode::OutOfMemory, "Failed to allocate GUI texture memory");
    }

    vkBindImageMemory(m_device, entry.image, entry.imageMemory, 0);
    return {};
}

Result<void> GuiTextureManager::_createImageView(ContainerTextureEntry& entry)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = entry.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &entry.imageView) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create GUI texture image view");
    }

    return {};
}

Result<void> GuiTextureManager::_createSampler(ContainerTextureEntry& entry)
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

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &entry.sampler) != VK_SUCCESS) {
        return Error(ErrorCode::InitializationFailed, "Failed to create GUI texture sampler");
    }

    return {};
}

Result<void> GuiTextureManager::_uploadTextureData(const std::vector<u8>& data, ContainerTextureEntry& entry)
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
    barrier.image = entry.image;
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
    region.imageExtent = {entry.width, entry.height, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, entry.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

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

Result<u32> GuiTextureManager::_findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    return VulkanUtils::findMemoryType(m_physicalDevice, typeFilter, properties);
}

VkCommandBuffer GuiTextureManager::_beginSingleTimeCommands()
{
    return VulkanUtils::beginSingleTimeCommands(m_device, m_commandPool);
}

void GuiTextureManager::_endSingleTimeCommands(VkCommandBuffer cmd)
{
    VulkanUtils::endSingleTimeCommands(m_device, m_commandPool, m_graphicsQueue, cmd);
}

} // namespace mc::client::renderer::trident::gui
