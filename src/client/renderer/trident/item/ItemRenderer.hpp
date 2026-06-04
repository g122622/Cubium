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
#include "common/resource/ResourceLocation.hpp"

namespace mc {

class Item;
class ItemStack;
struct TextureRegion;
class ResourceManager;

namespace client {
class ItemTextureAtlas;
}

namespace client::renderer::trident::gui {
class GuiRenderer;
}

namespace client::renderer::trident::item {

/**
 * @brief 物品渲染器
 *
 * 负责在GUI中渲染物品图标。
 *
 * 纹理来源：
 * - 统一使用物品纹理图集（ItemTextureAtlas）
 * - 方块物品纹理在构建图集时回退到 block 纹理并建立别名
 *
 * 用法：
 * @code
 * ItemRenderer itemRenderer;
 * itemRenderer.initialize(resourceManager, itemTextureAtlas);
 *
 * // 在GUI渲染中
 * itemRenderer.renderItem(gui, itemStack, x, y);
 * @endcode
 */
class ItemRenderer {
public:
    ItemRenderer();
    ~ItemRenderer() = default;

    // 禁止拷贝
    ItemRenderer(const ItemRenderer&) = delete;
    ItemRenderer& operator=(const ItemRenderer&) = delete;

    /**
     * @brief 初始化物品渲染器
     *
     * @param resourceManager 资源管理器（用于获取方块纹理）
     * @param itemTextureAtlas 物品纹理图集（用于非方块物品）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> initialize(ResourceManager* resourceManager, ItemTextureAtlas* itemTextureAtlas);

    /**
     * @brief 渲染物品图标
     *
     * @param gui GUI渲染器
     * @param stack 物品堆
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     * @param size 渲染尺寸（默认16像素）
     */
    void renderItem(gui::GuiRenderer& gui, const ItemStack& stack, f64 x, f64 y, f64 size = 16.0);

    /**
     * @brief 渲染物品图标（无数量显示）
     *
     * @param gui GUI渲染器
     * @param item 物品指针
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     * @param size 渲染尺寸（默认16像素）
     */
    void renderItem(gui::GuiRenderer& gui, const Item* item, f64 x, f64 y, f64 size = 16.0);

    /**
     * @brief 渲染物品图标（使用纹理区域）
     *
     * 直接使用纹理区域渲染，用于自定义纹理。
     *
     * @param gui GUI渲染器
     * @param region 纹理区域
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     * @param size 渲染尺寸（默认16像素）
     */
    void renderItem(gui::GuiRenderer& gui, const TextureRegion& region, f64 x, f64 y, f64 size = 16.0);

    /**
     * @brief 检查物品是否为方块物品
     *
     * 方块物品使用方块纹理图集，非方块物品使用物品纹理图集。
     *
     * @param item 物品指针
     * @return 是否为方块物品
     */
    [[nodiscard]] bool isBlockItem(const Item* item) const;

    /**
     * @brief 获取物品的纹理区域
     *
     * @param item 物品指针
     * @return 纹理区域指针，如果不存在返回 nullptr
     */
    [[nodiscard]] const TextureRegion* getItemTextureRegion(const Item* item) const;

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    ResourceManager* m_resourceManager = nullptr; // TODO: 当前未使用，待后续方块模型渲染等功能需要时启用
    ItemTextureAtlas* m_itemTextureAtlas = nullptr;
    bool m_initialized = false;
};

} // namespace client::renderer::trident::item

} // namespace mc

namespace mc::client {
// 向后兼容别名
using ItemRenderer = renderer::trident::item::ItemRenderer;
} // namespace mc::client
