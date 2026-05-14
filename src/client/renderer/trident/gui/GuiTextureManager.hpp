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
#include <vulkan/vulkan.h>

namespace mc {
class ResourceManager;
}

namespace mc::client::renderer::trident::gui {

// 前向声明
class GuiRenderer;
class GuiTextureAtlas;

/**
 * @brief GUI容器纹理UV坐标常量
 *
 * 基于 inventory.png (256x256) 纹理的UV坐标。
 * 参考 MC 1.16.5 ContainerScreen.java
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
} // namespace ContainerTex

/**
 * @brief GUI纹理管理器
 *
 * 统一管理所有GUI容器纹理的加载、缓存和渲染。
 * 负责加载 inventory.png 等GUI纹理并注册到GuiRenderer。
 *
 * 使用示例：
 * @code
 * GuiTextureManager textureManager;
 * textureManager.initialize(device, physicalDevice, commandPool, graphicsQueue, resourceManager);
 * textureManager.loadInventoryTexture();
 * textureManager.registerToRenderer(guiRenderer);
 *
 * // 绘制背包背景
 * textureManager.drawInventoryBackground(gui, x, y);
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
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadCraftingTableTexture();

    /**
     * @brief 注册到GuiRenderer
     *
     * 将加载的纹理注册到GuiRenderer的多图集槽位。
     *
     * @param renderer GUI渲染器
     * @return 分配的图集槽位ID，失败返回错误
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
     * @brief 检查背包纹理是否已加载
     */
    [[nodiscard]] bool hasInventoryTexture() const { return m_inventoryLoaded; }

    /**
     * @brief 检查工作台纹理是否已加载
     */
    [[nodiscard]] bool hasCraftingTableTexture() const { return m_craftingTableLoaded; }

    /**
     * @brief 获取图集槽位ID
     */
    [[nodiscard]] u8 atlasSlot() const { return m_atlasSlot; }

    /**
     * @brief 获取纹理图集图像视图
     */
    [[nodiscard]] VkImageView imageView() const { return m_imageView; }

    /**
     * @brief 获取纹理图集采样器
     */
    [[nodiscard]] VkSampler sampler() const { return m_sampler; }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    /**
     * @brief 创建默认纹理（无资源包时使用）
     */
    [[nodiscard]] Result<void> createDefaultTextures();

    /**
     * @brief 创建图像
     */
    [[nodiscard]] Result<void> createImage(u32 width, u32 height);

    /**
     * @brief 创建图像视图
     */
    [[nodiscard]] Result<void> createImageView();

    /**
     * @brief 创建采样器
     */
    [[nodiscard]] Result<void> createSampler();

    /**
     * @brief 上传纹理数据
     */
    [[nodiscard]] Result<void> uploadTextureData(const std::vector<u8>& data);

    /**
     * @brief 查找内存类型
     */
    [[nodiscard]] Result<u32> findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);

    /**
     * @brief 开始单次命令
     */
    VkCommandBuffer beginSingleTimeCommands();

    /**
     * @brief 结束单次命令
     */
    void endSingleTimeCommands(VkCommandBuffer cmd);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    ResourceManager* m_resourceManager = nullptr;

    // 纹理资源
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;

    // 图集尺寸
    u32 m_width = 256;
    u32 m_height = 256;

    // 图集槽位（由GuiRenderer分配）
    u8 m_atlasSlot = 255;

    // 纹理加载状态
    bool m_initialized = false;
    bool m_inventoryLoaded = false;
    bool m_craftingTableLoaded = false;
};

} // namespace mc::client::renderer::trident::gui

// 向后兼容别名
namespace mc::client {
using GuiTextureManager = renderer::trident::gui::GuiTextureManager;
}
