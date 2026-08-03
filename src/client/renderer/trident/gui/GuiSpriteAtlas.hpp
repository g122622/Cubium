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

#include "client/renderer/trident/gui/GuiSprite.hpp"
#include "client/renderer/trident/gui/ISpriteAtlas.hpp"
#include "client/ui/kagero/paint/TextureImage.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident::gui {

// 前向声明
class GuiTextureAtlas;
class GuiSpriteManager;

/**
 * @brief GUI精灵图集整合类
 *
 * 整合精灵数据管理和Vulkan纹理资源，提供统一的精灵访问接口。
 * 用于HUD、容器屏幕等GUI组件的纹理绘制。
 *
 * 主要功能：
 * - 管理GUI精灵定义（心形、饥饿、快捷栏等）
 * - 提供Vulkan纹理资源（VkImageView、VkSampler）
 * - 创建TextureImage用于PaintContext绘制
 *
 * 使用示例：
 * @code
 * GuiSpriteAtlas atlas;
 * atlas.initialize(device, physicalDevice, commandPool, graphicsQueue);
 * atlas.loadDefaultTextures();
 * GuiSpriteRegistry::registerAllDefaults(atlas);
 *
 * // 在Widget中使用
 * auto image = atlas.createTextureImage("heart_full");
 * if (image.isValid()) {
 *     ctx.drawImage(image, x, y);
 * }
 * @endcode
 */
class GuiSpriteAtlas : public ISpriteAtlas {
public:
    GuiSpriteAtlas();
    ~GuiSpriteAtlas() override;

    // 禁止拷贝（持有Vulkan资源）
    GuiSpriteAtlas(const GuiSpriteAtlas&) = delete;
    GuiSpriteAtlas& operator=(const GuiSpriteAtlas&) = delete;

    // 允许移动
    GuiSpriteAtlas(GuiSpriteAtlas&&) noexcept;
    GuiSpriteAtlas& operator=(GuiSpriteAtlas&&) noexcept;

    // ==================== 初始化与销毁 ====================

    /**
     * @brief 初始化精灵图集
     *
     * 创建Vulkan纹理资源和采样器。
     * 必须在使用任何其他方法之前调用。
     *
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
     *
     * 释放所有Vulkan资源。
     */
    void destroy();

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    // ==================== 纹理加载 ====================

    /**
     * @brief 加载默认GUI纹理
     *
     * 加载内置的GUI纹理图集（icons.png, widgets.png等）。
     * 如果资源文件不存在，会创建程序生成的默认纹理。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadDefaultTextures();

    /**
     * @brief 从文件加载纹理图集
     *
     * @param filePath 纹理文件路径
     * @param atlasWidth 图集宽度
     * @param atlasHeight 图集高度
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadTextureAtlas(const std::string& filePath, i32 atlasWidth, i32 atlasHeight);

    /**
     * @brief 从内存数据加载纹理
     *
     * 从RGBA像素数据创建Vulkan纹理。这是加载纹理的核心方法。
     *
     * @param pixels RGBA像素数据（每像素4字节）
     * @param width 纹理宽度
     * @param height 纹理高度
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadTextureFromMemory(const std::vector<u8>& pixels, i32 width, i32 height);

    /**
     * @brief 检查纹理是否已加载
     *
     * @return 如果纹理已成功加载返回true
     */
    [[nodiscard]] bool hasTexture() const;

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
     * @param atlasWidth 图集总宽度（像素），默认使用当前图集宽度
     * @param atlasHeight 图集总高度（像素），默认使用当前图集高度
     */
    void registerSprite(
        const std::string& id, i32 x, i32 y, i32 width, i32 height, i32 atlasWidth = 0, i32 atlasHeight = 0);

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
    [[nodiscard]] const GuiSprite* getSprite(const std::string& id) const override;

    /**
     * @brief 检查精灵是否存在
     * @param id 精灵ID
     * @return 如果精灵存在返回true
     */
    [[nodiscard]] bool hasSprite(const std::string& id) const override;

    /**
     * @brief 清除所有精灵
     */
    void clearSprites();

    /**
     * @brief 获取精灵数量
     */
    [[nodiscard]] size_t spriteCount() const;

    /**
     * @brief 设置图集尺寸
     * @param width 图集宽度
     * @param height 图集高度
     */
    void setAtlasSize(i32 width, i32 height);

    /**
     * @brief 获取图集宽度
     */
    [[nodiscard]] i32 atlasWidth() const;

    /**
     * @brief 获取图集高度
     */
    [[nodiscard]] i32 atlasHeight() const;

    // ==================== Vulkan资源访问 ====================

    /**
     * @brief 获取纹理图集图像视图
     */
    [[nodiscard]] VkImageView imageView() const;

    /**
     * @brief 获取纹理图集采样器
     */
    [[nodiscard]] VkSampler sampler() const;

    // ==================== 图集槽位 ====================

    /**
     * @brief 设置图集槽位ID
     *
     * 槽位ID用于在多图集渲染时选择正确的纹理采样器。
     *
     * @param slot 槽位ID（2-15，0=字体，1=物品）
     */
    void setAtlasSlot(u8 slot) { m_atlasSlot = slot; }

    /**
     * @brief 获取图集槽位ID
     */
    [[nodiscard]] u8 atlasSlot() const { return m_atlasSlot; }

    // ==================== TextureImage创建 ====================

    /**
     * @brief 创建TextureImage用于PaintContext绘制
     *
     * 创建一个轻量级的TextureImage对象，持有对图集纹理的引用。
     * 可以直接传递给PaintContext::drawImage()进行绘制。
     *
     * 注意：返回的TextureImage不拥有纹理资源，
     * 纹理生命周期由GuiSpriteAtlas管理。
     *
     * @param spriteId 精灵ID
     * @return TextureImage对象，如果精灵不存在则返回无效对象
     */
    [[nodiscard]] ui::kagero::paint::TextureImage createTextureImage(const std::string& spriteId) const override;

    /**
     * @brief 创建TextureImage（带自定义尺寸）
     *
     * @param spriteId 精灵ID
     * @param customWidth 自定义绘制宽度
     * @param customHeight 自定义绘制高度
     * @return TextureImage对象
     */
    [[nodiscard]] ui::kagero::paint::TextureImage createTextureImage(
        const std::string& spriteId, i32 customWidth, i32 customHeight) const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    bool m_initialized = false;
    u8 m_atlasSlot = 2; ///< 图集槽位ID（默认使用槽位2，GUI图集起始槽位）
};

} // namespace mc::client::renderer::trident::gui
