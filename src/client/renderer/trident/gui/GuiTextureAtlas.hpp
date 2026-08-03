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

#include "GuiSprite.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident::gui {

// 前向声明
class GuiRenderer;

/**
 * @brief GUI纹理区域（向后兼容）
 *
 * 定义GUI纹理图集中的单个纹理区域。
 * @deprecated 请使用 GuiSprite 代替
 */
struct GuiTextureRegion {
    f64 u0, v0; ///< 左上角UV坐标
    f64 u1, v1; ///< 右下角UV坐标
    i32 width;  ///< 纹理宽度（像素）
    i32 height; ///< 纹理高度（像素）
};

/**
 * @brief GUI纹理图集
 *
 * 管理所有GUI纹理的图集，包括：
 * - 容器背景纹理
 * - 槽位背景
 * - 图标
 * - 按钮纹理
 *
 * 支持精灵（Sprite）管理，可以从资源包加载纹理并注册精灵区域。
 *
 * 使用示例：
 * @code
 * GuiTextureAtlas atlas;
 * atlas.initialize(device, physicalDevice, commandPool, graphicsQueue);
 *
 * // 注册精灵（图集尺寸256x256）
 * atlas.registerSprite("button_normal", 0, 66, 200, 20, 256, 256);
 * atlas.registerSprite("button_hover", 0, 86, 200, 20, 256, 256);
 *
 * // 绘制精灵
 * atlas.drawSprite(gui, "button_normal", x, y, 200, 20);
 * @endcode
 */
class GuiTextureAtlas {
public:
    GuiTextureAtlas();
    ~GuiTextureAtlas();

    // 禁止拷贝
    GuiTextureAtlas(const GuiTextureAtlas&) = delete;
    GuiTextureAtlas& operator=(const GuiTextureAtlas&) = delete;

    // 允许移动
    GuiTextureAtlas(GuiTextureAtlas&&) noexcept = default;
    GuiTextureAtlas& operator=(GuiTextureAtlas&&) noexcept = default;

    /**
     * @brief 初始化GUI纹理图集
     * @param device Vulkan设备
     * @param physicalDevice 物理设备
     * @param commandPool 命令池
     * @param graphicsQueue 图形队列
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(
        VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);

    /**
     * @brief 销毁资源
     */
    void destroy();

    /**
     * @brief 加载GUI纹理
     * @param location 纹理资源位置
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadGuiTexture(const ResourceLocation& location);

    /**
     * @brief 加载默认GUI纹理
     *
     * 加载内置的GUI纹理：
     * - 容器背景
     * - 槽位背景
     * - 图标
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadDefaultTextures();

    /**
     * @brief 绘制完整纹理
     * @param gui GUI渲染器
     * @param textureId 纹理ID
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     * @param width 绘制宽度
     * @param height 绘制高度
     */
    void drawTexture(GuiRenderer& gui, const std::string& textureId, f64 x, f64 y, f64 width, f64 height);

    /**
     * @brief 绘制纹理区域
     * @param gui GUI渲染器
     * @param textureId 纹理ID
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     * @param region 纹理区域（像素）
     * @param width 绘制宽度
     * @param height 绘制高度
     */
    void drawTextureRegion(GuiRenderer& gui,
        const std::string& textureId,
        f64 x,
        f64 y,
        i32 regionX,
        i32 regionY,
        i32 regionWidth,
        i32 regionHeight,
        f64 width,
        f64 height);

    /**
     * @brief 检查纹理是否已加载
     */
    [[nodiscard]] bool hasTexture(const std::string& textureId) const;

    /**
     * @brief 获取纹理区域
     * @param textureId 纹理ID
     * @return 纹理区域，如果不存在返回nullptr
     */
    [[nodiscard]] const GuiTextureRegion* getRegion(const std::string& textureId) const;

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

    // ==================== 精灵管理 ====================

    /**
     * @brief 注册精灵
     *
     * @param sprite 精灵定义
     */
    void registerSprite(const GuiSprite& sprite);

    /**
     * @brief 注册精灵（便捷方法）
     *
     * @param id 精灵ID
     * @param x 纹理中的X坐标（像素）
     * @param y 纹理中的Y坐标（像素）
     * @param width 精灵宽度（像素）
     * @param height 精灵高度（像素）
     * @param atlasWidth 图集总宽度（像素）
     * @param atlasHeight 图集总高度（像素）
     */
    void registerSprite(const std::string& id, i32 x, i32 y, i32 width, i32 height, i32 atlasWidth, i32 atlasHeight);

    /**
     * @brief 批量注册精灵
     * @param sprites 精灵列表
     */
    void registerSprites(const std::vector<GuiSprite>& sprites);

    /**
     * @brief 获取精灵
     * @param id 精灵ID
     * @return 精灵指针，如果不存在返回nullptr
     */
    [[nodiscard]] const GuiSprite* getSprite(const std::string& id) const;

    /**
     * @brief 检查精灵是否存在
     * @param id 精灵ID
     * @return 如果精灵存在返回true
     */
    [[nodiscard]] bool hasSprite(const std::string& id) const;

    /**
     * @brief 清除所有精灵
     */
    void clearSprites();

    /**
     * @brief 获取精灵数量
     */
    [[nodiscard]] size_t spriteCount() const { return m_sprites.size(); }

    /**
     * @brief 设置当前图集尺寸（用于UV计算）
     * @param width 图集宽度
     * @param height 图集高度
     */
    void setAtlasSize(i32 width, i32 height);

    /**
     * @brief 获取图集宽度
     */
    [[nodiscard]] i32 atlasWidth() const { return m_atlasWidth; }

    /**
     * @brief 获取图集高度
     */
    [[nodiscard]] i32 atlasHeight() const { return m_atlasHeight; }

private:
    /**
     * @brief 创建默认纹理（用于无纹理资源包时）
     */
    [[nodiscard]] Result<void> _createDefaultTextures();

    /**
     * @brief 创建槽位背景纹理
     */
    void _createSlotBackground(u8* data, i32 width, i32 height);

    /**
     * @brief 创建容器背景纹理
     */
    void _createContainerBackground(u8* data, i32 width, i32 height);

    /**
     * @brief 创建图像
     */
    [[nodiscard]] Result<void> _createImage(u32 width, u32 height);

    /**
     * @brief 创建图像视图
     */
    [[nodiscard]] Result<void> _createImageView();

    /**
     * @brief 创建采样器
     */
    [[nodiscard]] Result<void> _createSampler();

    /**
     * @brief 上传纹理数据
     */
    [[nodiscard]] Result<void> _uploadTextureData(const std::vector<u8>& data);

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

    // 纹理资源
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;

    // 图集尺寸
    u32 m_width = 0;
    u32 m_height = 0;
    i32 m_atlasWidth = DEFAULT_ATLAS_WIDTH;   ///< 用于精灵UV计算的图集宽度
    i32 m_atlasHeight = DEFAULT_ATLAS_HEIGHT; ///< 用于精灵UV计算的图集高度

    // 纹理区域映射（向后兼容）
    std::unordered_map<std::string, GuiTextureRegion> m_regions;

    // 精灵映射
    std::unordered_map<std::string, GuiSprite> m_sprites;

    // 默认纹理尺寸
    static constexpr i32 DEFAULT_SLOT_SIZE = 18;
    static constexpr i32 DEFAULT_CONTAINER_WIDTH = 176;
    static constexpr i32 DEFAULT_CONTAINER_HEIGHT = 166;

    // 默认图集尺寸
    static constexpr i32 DEFAULT_ATLAS_WIDTH = 256;
    static constexpr i32 DEFAULT_ATLAS_HEIGHT = 256;

    bool m_initialized = false;
};

} // namespace mc::client::renderer::trident::gui

// 向后兼容别名
namespace mc::client {
using GuiTextureAtlas = renderer::trident::gui::GuiTextureAtlas;
using GuiTextureRegion = renderer::trident::gui::GuiTextureRegion;
} // namespace mc::client
