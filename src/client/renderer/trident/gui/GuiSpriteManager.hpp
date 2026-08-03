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
#include "common/core/Types.hpp"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::client::renderer::trident::gui {

/**
 * @brief GUI精灵管理器（不依赖Vulkan）
 *
 * 纯数据结构，管理GUI精灵的注册、查询和删除。
 * 可以独立于Vulkan使用，适合单元测试和资源加载阶段。
 *
 * 使用示例：
 * @code
 * GuiSpriteManager manager;
 * manager.registerSprite("button_normal", 0, 66, 200, 20, 256, 256);
 *
 * const GuiSprite* sprite = manager.getSprite("button_normal");
 * if (sprite) {
 *     // 使用精灵...
 * }
 * @endcode
 */
class GuiSpriteManager {
public:
    GuiSpriteManager() = default;
    ~GuiSpriteManager() = default;

    // 允许拷贝和移动
    GuiSpriteManager(const GuiSpriteManager&) = default;
    GuiSpriteManager& operator=(const GuiSpriteManager&) = default;
    GuiSpriteManager(GuiSpriteManager&&) noexcept = default;
    GuiSpriteManager& operator=(GuiSpriteManager&&) noexcept = default;

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

    /**
     * @brief 获取所有精灵（只读）
     */
    [[nodiscard]] const std::unordered_map<std::string, GuiSprite>& sprites() const { return m_sprites; }

private:
    std::unordered_map<std::string, GuiSprite> m_sprites;
    i32 m_atlasWidth = 256;  ///< 用于精灵UV计算的图集宽度
    i32 m_atlasHeight = 256; ///< 用于精灵UV计算的图集高度
};

} // namespace mc::client::renderer::trident::gui
