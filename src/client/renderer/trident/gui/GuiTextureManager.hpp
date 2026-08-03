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

#include "client/renderer/MeshTypes.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc {
class ResourceManager;
}

namespace mc::client::renderer::trident::gui {

// 前向声明
class GuiRenderer;
class GuiTextureAtlas;

/**
 * @brief GUI容器颜色常量
 *
 * 用于程序化默认纹理生成和纯色后备绘制。
 */
namespace GuiColors {
constexpr u32 CONTAINER_BG = 0xFFC6C6C6;       // 浅灰背景
constexpr u32 CONTAINER_BORDER = 0xFF555555;   // 深灰边框
constexpr u32 SLOT_BG = 0xFF8B8B8B;            // 槽位背景
constexpr u32 SLOT_BORDER = 0xFF373737;        // 槽位边框
constexpr u32 DEFAULT_BG = 0xFF404040;         // 默认背景
constexpr u32 FURNACE_FIRE_FILL = 0xFFFFAA00;  // 火焰填充颜色（橙色）
constexpr u32 FURNACE_ARROW_FILL = 0xFFC6C6C6; // 箭头填充颜色（浅灰）
} // namespace GuiColors

/**
 * @brief GUI容器纹理UV坐标常量
 *
 * 基于 256x256 纹理的UV坐标。所有MC容器纹理（inventory.png、
 * crafting_table.png、furnace.png等）均为 256x256 像素，GUI可见区域
 * 为左上角 176x166 像素。
 */
namespace ContainerTex {
// 纹理尺寸
constexpr i32 TEXTURE_WIDTH = 256;
constexpr i32 TEXTURE_HEIGHT = 256;

// 背包屏幕背景 (0, 0) - (176, 166)
constexpr f64 INVENTORY_BG_U0 = 0.0f / TEXTURE_WIDTH;
constexpr f64 INVENTORY_BG_V0 = 0.0f / TEXTURE_HEIGHT;
constexpr f64 INVENTORY_BG_U1 = 176.0f / TEXTURE_WIDTH;
constexpr f64 INVENTORY_BG_V1 = 166.0f / TEXTURE_HEIGHT;
constexpr i32 INVENTORY_BG_WIDTH = 176;
constexpr i32 INVENTORY_BG_HEIGHT = 166;

// 工作台背景 (0, 0) - (176, 166)
constexpr f64 CRAFTING_TABLE_BG_U0 = 0.0f / TEXTURE_WIDTH;
constexpr f64 CRAFTING_TABLE_BG_V0 = 0.0f / TEXTURE_HEIGHT;
constexpr f64 CRAFTING_TABLE_BG_U1 = 176.0f / TEXTURE_WIDTH;
constexpr f64 CRAFTING_TABLE_BG_V1 = 166.0f / TEXTURE_HEIGHT;
constexpr i32 CRAFTING_TABLE_BG_WIDTH = 176;
constexpr i32 CRAFTING_TABLE_BG_HEIGHT = 166;

// 熔炉屏幕背景 (0, 0) - (176, 166)
constexpr f64 FURNACE_BG_U0 = 0.0f / TEXTURE_WIDTH;
constexpr f64 FURNACE_BG_V0 = 0.0f / TEXTURE_HEIGHT;
constexpr f64 FURNACE_BG_U1 = 176.0f / TEXTURE_WIDTH;
constexpr f64 FURNACE_BG_V1 = 166.0f / TEXTURE_HEIGHT;
constexpr i32 FURNACE_BG_WIDTH = 176;
constexpr i32 FURNACE_BG_HEIGHT = 166;

// 熔炉火焰指示器（lit_progress），位于 furnace.png 右侧
// 在经典 256x256 纹理中，火焰图标位于像素坐标 (176, 0)，尺寸 14x14
constexpr f64 FURNACE_LIT_U0 = 176.0f / TEXTURE_WIDTH;
constexpr f64 FURNACE_LIT_V0 = 0.0f / TEXTURE_HEIGHT;
constexpr f64 FURNACE_LIT_U1 = 190.0f / TEXTURE_WIDTH;
constexpr f64 FURNACE_LIT_V1 = 14.0f / TEXTURE_HEIGHT;
constexpr i32 FURNACE_LIT_WIDTH = 14;
constexpr i32 FURNACE_LIT_HEIGHT = 14;

// 熔炉火焰在屏幕上的位置（相对于GUI左上角）
constexpr i32 FURNACE_LIT_SCREEN_X = 56;
constexpr i32 FURNACE_LIT_SCREEN_Y = 36;

// 熔炉进度箭头（burn_progress），位于 furnace.png 右侧
// 在经典 256x256 纹理中，箭头图标位于像素坐标 (176, 14)，尺寸 24x16
constexpr f64 FURNACE_ARROW_U0 = 176.0f / TEXTURE_WIDTH;
constexpr f64 FURNACE_ARROW_V0 = 14.0f / TEXTURE_HEIGHT;
constexpr f64 FURNACE_ARROW_U1 = 200.0f / TEXTURE_WIDTH;
constexpr f64 FURNACE_ARROW_V1 = 30.0f / TEXTURE_HEIGHT;
constexpr i32 FURNACE_ARROW_WIDTH = 24;
constexpr i32 FURNACE_ARROW_HEIGHT = 16;

// 熔炉箭头在屏幕上的位置（相对于GUI左上角）
constexpr i32 FURNACE_ARROW_SCREEN_X = 79;
constexpr i32 FURNACE_ARROW_SCREEN_Y = 34;
} // namespace ContainerTex

/**
 * @brief 单个容器纹理资源
 *
 * 管理一个容器纹理的Vulkan资源（图像、视图、采样器、图集槽位）。
 */
struct ContainerTextureEntry {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    u32 width = 256;
    u32 height = 256;
    u8 atlasSlot = 255; ///< 255 = 未注册
    bool loaded = false;
};

/**
 * @brief GUI纹理管理器
 *
 * 统一管理所有GUI容器纹理的加载、缓存和渲染。
 * 负责加载 inventory.png、furnace.png 等GUI纹理并注册到GuiRenderer。
 * 每种容器纹理拥有独立的Vulkan资源和图集槽位。
 *
 * 使用示例：
 * @code
 * GuiTextureManager textureManager;
 * textureManager.initialize(device, physicalDevice, commandPool, graphicsQueue, resourceManager);
 * textureManager.loadInventoryTexture();
 * textureManager.loadFurnaceTexture();
 * textureManager.registerToRenderer(guiRenderer);
 *
 * // 绘制背包背景
 * textureManager.drawInventoryBackground(gui, x, y);
 *
 * // 绘制熔炉背景
 * textureManager.drawFurnaceBackground(gui, x, y);
 * @endcode
 */
class GuiTextureManager {
public:
    GuiTextureManager();
    ~GuiTextureManager();

    // 禁止拷贝
    GuiTextureManager(const GuiTextureManager&) = delete;
    GuiTextureManager& operator=(const GuiTextureManager&) = delete;

    // 允许移动
    GuiTextureManager(GuiTextureManager&&) noexcept = default;
    GuiTextureManager& operator=(GuiTextureManager&&) noexcept = default;

    /**
     * @brief 初始化GUI纹理管理器
     *
     * @param device Vulkan设备
     * @param physicalDevice 物理设备
     * @param commandPool 命令池
     * @param graphicsQueue 图形队列
     * @param resourceManager 资源管理器（可为nullptr，使用默认纹理）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        ResourceManager* resourceManager);

    /**
     * @brief 销毁资源
     */
    void destroy();

    /**
     * @brief 加载背包屏幕纹理
     *
     * 加载 minecraft:textures/gui/container/inventory.png
     * 如果资源不存在，使用程序生成的默认纹理。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadInventoryTexture();

    /**
     * @brief 加载工作台屏幕纹理
     *
     * 加载 minecraft:textures/gui/container/crafting_table.png
     * 如果资源不存在，使用背包纹理作为后备。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadCraftingTableTexture();

    /**
     * @brief 加载熔炉屏幕纹理
     *
     * 加载 minecraft:textures/gui/container/furnace.png
     * 纹理包含背景、火焰指示器和进度箭头区域。
     * 如果资源不存在，使用程序生成的默认纹理。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadFurnaceTexture();

    /**
     * @brief 注册所有已加载的纹理到GuiRenderer
     *
     * 将每种已加载的容器纹理注册到GuiRenderer的多图集槽位。
     * 需要在所有load*Texture()调用之后调用。
     *
     * @param renderer GUI渲染器
     * @return 分配的图集槽位数量，失败返回错误
     */
    [[nodiscard]] Result<u32> registerToRenderer(GuiRenderer& renderer);

    /**
     * @brief 绘制背包屏幕背景
     *
     * @param gui GUI渲染器
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     */
    void drawInventoryBackground(GuiRenderer& gui, f64 x, f64 y);

    /**
     * @brief 绘制工作台屏幕背景
     *
     * @param gui GUI渲染器
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     */
    void drawCraftingTableBackground(GuiRenderer& gui, f64 x, f64 y);

    /**
     * @brief 绘制熔炉屏幕背景
     *
     * 绘制熔炉的完整背景纹理（176x166区域）。
     *
     * @param gui GUI渲染器
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     */
    void drawFurnaceBackground(GuiRenderer& gui, f64 x, f64 y);

    /**
     * @brief 绘制熔炉火焰指示器
     *
     * 绘制熔炉燃料燃烧火焰指示器。火焰从底部向上填充，
     * litProgress为1.0时显示完整火焰，接近0时仅显示底部一行。
     *
     * @param gui GUI渲染器
     * @param x 屏幕X坐标（GUI左边缘）
     * @param y 屏幕Y坐标（GUI上边缘）
     * @param litProgress 燃烧进度 0.0~1.0
     */
    void drawFurnaceLitProgress(GuiRenderer& gui, f64 x, f64 y, f32 litProgress);

    /**
     * @brief 绘制熔炉进度箭头
     *
     * 绘制熔炼进度箭头。箭头从左向右填充，
     * burnProgress为1.0时显示完整箭头，0时不显示。
     *
     * @param gui GUI渲染器
     * @param x 屏幕X坐标（GUI左边缘）
     * @param y 屏幕Y坐标（GUI上边缘）
     * @param burnProgress 熔炼进度 0.0~1.0
     */
    void drawFurnaceBurnProgress(GuiRenderer& gui, f64 x, f64 y, f32 burnProgress);

    /**
     * @brief 检查背包纹理是否已加载
     */
    [[nodiscard]] bool hasInventoryTexture() const { return m_inventoryEntry.loaded; }

    /**
     * @brief 检查工作台纹理是否已加载
     */
    [[nodiscard]] bool hasCraftingTableTexture() const { return m_craftingTableEntry.loaded; }

    /**
     * @brief 检查熔炉纹理是否已加载
     */
    [[nodiscard]] bool hasFurnaceTexture() const { return m_furnaceEntry.loaded; }

    /**
     * @brief 获取背包纹理图集槽位ID
     */
    [[nodiscard]] u8 atlasSlot() const { return m_inventoryEntry.atlasSlot; }

    /**
     * @brief 获取背包纹理图集槽位ID（向后兼容）
     */
    [[nodiscard]] u8 inventoryAtlasSlot() const { return m_inventoryEntry.atlasSlot; }

    /**
     * @brief 获取熔炉纹理图集槽位ID
     */
    [[nodiscard]] u8 furnaceAtlasSlot() const { return m_furnaceEntry.atlasSlot; }

    /**
     * @brief 获取背包纹理图集图像视图
     */
    [[nodiscard]] VkImageView imageView() const { return m_inventoryEntry.imageView; }

    /**
     * @brief 获取背包纹理图集采样器
     */
    [[nodiscard]] VkSampler sampler() const { return m_inventoryEntry.sampler; }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    /**
     * @brief 从资源加载纹理并创建Vulkan资源
     *
     * @param resourcePath 资源路径（如 "minecraft:textures/gui/container/furnace"）
     * @param entry 输出的纹理条目
     * @param createDefault 是否在加载失败时创建默认纹理
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> _loadTexture(
        const std::string& resourcePath, ContainerTextureEntry& entry, bool createDefault);

    /**
     * @brief 创建默认容器背景纹理（无资源包时使用）
     *
     * @param entry 输出的纹理条目
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> _createDefaultContainerTexture(ContainerTextureEntry& entry);

    /**
     * @brief 创建熔炉默认纹理
     *
     * 生成包含背景区域和火焰/箭头指示器区域的程序化纹理。
     *
     * @param entry 输出的纹理条目
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> _createDefaultFurnaceTexture(ContainerTextureEntry& entry);

    /**
     * @brief 为纹理条目创建Vulkan图像
     */
    [[nodiscard]] Result<void> _createImage(u32 width, u32 height, ContainerTextureEntry& entry);

    /**
     * @brief 为纹理条目创建图像视图
     */
    [[nodiscard]] Result<void> _createImageView(ContainerTextureEntry& entry);

    /**
     * @brief 为纹理条目创建采样器
     */
    [[nodiscard]] Result<void> _createSampler(ContainerTextureEntry& entry);

    /**
     * @brief 上传纹理数据到指定条目
     */
    [[nodiscard]] Result<void> _uploadTextureData(const std::vector<u8>& data, ContainerTextureEntry& entry);

    /**
     * @brief 销毁单个纹理条目的Vulkan资源
     */
    void _destroyEntry(ContainerTextureEntry& entry);

    /**
     * @brief 查找内存类型
     */
    [[nodiscard]] Result<u32> _findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);

    /**
     * @brief 开始单次命令
     */
    VkCommandBuffer _beginSingleTimeCommands();

    /**
     * @brief 结束单次命令
     */
    void _endSingleTimeCommands(VkCommandBuffer cmd);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    ResourceManager* m_resourceManager = nullptr;

    // 每种容器类型拥有独立的纹理资源
    ContainerTextureEntry m_inventoryEntry;     ///< 背包纹理
    ContainerTextureEntry m_craftingTableEntry; ///< 工作台纹理
    ContainerTextureEntry m_furnaceEntry;       ///< 熔炉纹理

    bool m_initialized = false;
};

} // namespace mc::client::renderer::trident::gui

// 向后兼容别名
namespace mc::client {
using GuiTextureManager = renderer::trident::gui::GuiTextureManager;
}
