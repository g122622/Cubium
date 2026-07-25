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

#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {
class Player;
class ItemStack;
} // namespace mc

namespace mc::client {
class MapRenderer;
class ClientMapDataCache;
} // namespace mc::client

namespace mc::client::renderer::trident::gui {
class GuiRenderer;
class GuiSpriteAtlas;
} // namespace mc::client::renderer::trident::gui

namespace mc::client::renderer::trident::item {
class ItemRenderer;
}

namespace mc::client::ui::minecraft::widgets {

/**
 * @brief HUD元素颜色常量
 */
namespace HudColors {
// 快捷栏
constexpr u32 HOTBAR_SLOT = 0xFF8B8B8B;           // 槽位背景
constexpr u32 HOTBAR_SLOT_HIGHLIGHT = 0xFFFFFFFF; // 选中槽位高亮
constexpr u32 HOTBAR_BACKGROUND = 0xFF000000;     // 背景
constexpr u32 HOTBAR_BORDER = 0xFF373737;         // 边框

// 生命值
constexpr u32 HEALTH_RED = 0xFFFF0000;    // 红心
constexpr u32 HEALTH_YELLOW = 0xFFFFF600; // 黄心（吸收）
constexpr u32 HEALTH_EMPTY = 0xFF2A0A0A;  // 空心

// 饥饿值
constexpr u32 HUNGER_FULL = 0xFFE0A010;   // 满饥饿
constexpr u32 HUNGER_EMPTY = 0xFF1A0A00;  // 空饥饿
constexpr u32 HUNGER_EFFECT = 0xFF60C010; // 饥饿效果变体（绿色调）

// 经验条
constexpr u32 XP_BACKGROUND = 0xFF202020; // 背景
constexpr u32 XP_FOREGROUND = 0xFF7FFF00; // 前景
constexpr u32 XP_TEXT = 0xFF7FFF00;       // 文字

// 物品提示
constexpr u32 TOOLTIP_BACKGROUND = 0xF0100010; // 背景带透明
constexpr u32 TOOLTIP_BORDER = 0xFF5000FF;     // 边框
constexpr u32 TOOLTIP_TEXT = 0xFFFFFFFF;       // 文字
} // namespace HudColors

/**
 * @brief HUD Widget
 *
 * 渲染游戏内HUD元素：
 * - 快捷栏
 * - 生命值
 * - 饥饿值
 * - 盔甲值
 * - 经验条
 */
class HudWidget : public kagero::widget::Widget {
public:
    /**
     * @brief 构造函数
     */
    HudWidget();

    /**
     * @brief 析构函数
     */
    ~HudWidget() override = default;

    /**
     * @brief 设置GUI渲染器（用于物品渲染）
     */
    void setGuiRenderer(renderer::trident::gui::GuiRenderer* gui) { m_gui = gui; }

    /**
     * @brief 设置物品渲染器
     */
    void setItemRenderer(renderer::trident::item::ItemRenderer* renderer) { m_itemRenderer = renderer; }

    /**
     * @brief 设置icons精灵图集（心形、饥饿、盔甲、经验条等）
     */
    void setIconsAtlas(renderer::trident::gui::GuiSpriteAtlas* atlas) { m_iconsAtlas = atlas; }

    /**
     * @brief 设置widgets精灵图集（快捷栏、按钮等）
     */
    void setWidgetsAtlas(renderer::trident::gui::GuiSpriteAtlas* atlas) { m_widgetsAtlas = atlas; }

    /**
     * @brief 设置玩家
     */
    void setPlayer(Player* player) { m_player = player; }

    /**
     * @brief 设置地图渲染器（用于手持地图内容的屏幕层绘制）
     */
    void setMapRenderer(MapRenderer* renderer) { m_mapRenderer = renderer; }

    /**
     * @brief 设置客户端地图数据缓存（手持地图内容来源）
     */
    void setMapDataCache(ClientMapDataCache* cache) { m_mapDataCache = cache; }

    /**
     * @brief 绘制HUD
     */
    void paint(kagero::widget::PaintContext& ctx) override;

    /**
     * @brief 设置HUD可见性
     * @note 使用基类的可见性功能
     */
    void setHudVisible(bool visible) { setVisible(visible); }

    /**
     * @brief 检查HUD是否可见
     * @note 使用基类的可见性功能
     */
    [[nodiscard]] bool isHudVisible() const { return isVisible(); }

private:
    /**
     * @brief 渲染快捷栏
     */
    void _renderHotbar(kagero::widget::PaintContext& ctx, renderer::trident::gui::GuiRenderer& gui);

    /**
     * @brief 渲染生命值和盔甲
     */
    void _renderHealth(kagero::widget::PaintContext& ctx);

    /**
     * @brief 渲染饥饿值
     */
    void _renderHunger(kagero::widget::PaintContext& ctx);

    /**
     * @brief 渲染经验条
     */
    void _renderExperience(kagero::widget::PaintContext& ctx);

    /**
     * @brief 渲染手持地图内容（屏幕层）
     *
     * 玩家主手持已填充地图时，在屏幕中央偏上绘制地图内容。
     * 双手举起姿态由 FirstPersonRenderer 负责，地图内容在此绘制（对齐 Java：
     * MapItemSavedData 内容由 GuiRendering 在屏幕层渲染）。
     */
    void _renderHeldMap();

    /**
     * @brief 绘制心形图标
     * @param ctx 绘制上下文
     * @param x X坐标
     * @param y Y坐标
     * @param full 是否满心
     * @param half 是否半心
     * @param absorbing 是否吸收心（黄色）
     */
    void _drawHeart(kagero::widget::PaintContext& ctx, f32 x, f32 y, bool full, bool half, bool absorbing);

    /**
     * @brief 绘制饥饿图标
     * @param ctx 绘制上下文
     * @param x X坐标
     * @param y Y坐标
     * @param full 是否满饥饿
     * @param half 是否半饥饿
     * @param hasHungerEffect 是否有饥饿效果（绿色变体图标）
     */
    void _drawHunger(kagero::widget::PaintContext& ctx, f32 x, f32 y, bool full, bool half, bool hasHungerEffect);

    /**
     * @brief 绘制盔甲图标
     * @param ctx 绘制上下文
     * @param x X坐标
     * @param y Y坐标
     * @param full 是否满盔甲
     */
    void _drawArmor(kagero::widget::PaintContext& ctx, f32 x, f32 y, bool full);

    /**
     * @brief 绘制经验条
     * @param ctx 绘制上下文
     * @param x X坐标
     * @param y Y坐标
     * @param progress 进度 (0.0-1.0)
     * @param width 宽度
     * @param height 高度
     */
    void _drawExperienceBar(kagero::widget::PaintContext& ctx, f32 x, f32 y, f32 progress, f32 width, f32 height);

    renderer::trident::gui::GuiRenderer* m_gui = nullptr;
    renderer::trident::item::ItemRenderer* m_itemRenderer = nullptr;
    renderer::trident::gui::GuiSpriteAtlas* m_iconsAtlas = nullptr;   // 心形、饥饿、盔甲、经验条
    renderer::trident::gui::GuiSpriteAtlas* m_widgetsAtlas = nullptr; // 快捷栏、按钮
    Player* m_player = nullptr;
    MapRenderer* m_mapRenderer = nullptr;         // 手持地图内容渲染
    ClientMapDataCache* m_mapDataCache = nullptr; // 地图数据来源

    // 动画状态
    mutable mc::math::Random m_random; ///< 确定性随机数生成器（用于饥饿条抖动偏移，每帧重设种子）
};

} // namespace mc::client::ui::minecraft::widgets
