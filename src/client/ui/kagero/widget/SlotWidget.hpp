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

#include <functional>
#include <string>
#include <utility>

#include "Widget.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"

namespace mc::client::ui::kagero::widget {

/**
 * @brief 物品槽组件
 *
 * 显示物品槽的组件，支持物品显示、交互和背景。
 *
 * 物品图标渲染采用回调注入（setItemPaintCallback），宿主屏幕注入一个
 * 调用 ItemRenderer::renderItem 的回调。这样 SlotWidget 本身不依赖
 * ItemRenderer/GuiRenderer 类型，保持 header-only 且无重渲染依赖。
 *
 * 使用示例：
 * @code
 * auto slot = std::make_unique<SlotWidget>("slot_0", 10, 10);
 * slot->setItem(itemStack);
 * slot->setItemPaintCallback([&](const ItemStack& it, i32 x, i32 y, i32 sz){
 *     itemRenderer.renderItem(gui, it, x, y, sz);
 * });
 * slot->setOnSlotClick([](i32 slotIndex, i32 button, bool shiftHeld) {
 *     // 处理点击
 * });
 * @endcode
 */
class SlotWidget : public Widget {
public:
    /**
     * @brief 槽位点击回调类型（与文档一致）
     *
     * 参数：slotIndex - 槽位索引
     *      button - 鼠标按钮
     *      shiftHeld - 是否按住Shift
     */
    using OnSlotClickCallback = std::function<void(i32, i32, bool)>;

    /**
     * @brief 槽位释放回调类型
     */
    using OnSlotReleaseCallback = std::function<void(SlotWidget&, i32)>;

    /**
     * @brief 物品图标绘制回调类型
     *
     * 宿主屏幕注入：给定物品与屏幕坐标/尺寸，绘制物品图标。
     * 典型实现为调用 ItemRenderer::renderItem。
     * 参数：item - 物品堆叠；x/y - 屏幕坐标；size - 绘制尺寸
     */
    using ItemPaintCallback = std::function<void(const mc::ItemStack& item, i32 x, i32 y, i32 size)>;

    /// 默认槽位尺寸（像素）
    static constexpr i32 DEFAULT_SLOT_SIZE = 16;

    /**
     * @brief 默认构造函数
     */
    SlotWidget() = default;

    /**
     * @brief 构造函数
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     */
    SlotWidget(std::string id, i32 x, i32 y)
        : Widget(std::move(id))
    {
        setBounds(Rect(x, y, DEFAULT_SLOT_SIZE, DEFAULT_SLOT_SIZE));
    }

    /**
     * @brief 构造函数（带尺寸）
     * @param id 组件ID
     * @param x X坐标
     * @param y Y坐标
     * @param size 尺寸（宽高相等）
     */
    SlotWidget(std::string id, i32 x, i32 y, i32 size)
        : Widget(std::move(id))
    {
        setBounds(Rect(x, y, size, size));
    }

    // ==================== 生命周期 ====================

    void paint(PaintContext& ctx) override
    {
        if (!isVisible()) return;

        if (m_showBackground) {
            ctx.drawFilledRect(bounds(), Colors::fromARGB(255, 40, 40, 40));
            // TODO: 使用m_backgroundTexture绘制背景纹理，当前仅绘制纯色背景
            ctx.drawBorder(bounds(), 1.0f, Colors::fromARGB(255, 100, 100, 100));
        }

        // 绘制槽位中的物品图标（经宿主注入的回调，复刻 HudWidget 的 ItemRenderer 注入模式）
        if (!m_item.isEmpty() && m_itemPaintCallback) {
            m_itemPaintCallback(m_item, bounds().x, bounds().y, DEFAULT_SLOT_SIZE);

            // 绘制物品数量（count > 1 时，右下角右对齐）
            if (m_showCount && m_item.getCount() > 1) {
                std::string countText = std::to_string(m_item.getCount());
                const f32 textWidth = ctx.getTextWidth(countText);
                ctx.drawText(countText,
                    bounds().x + DEFAULT_SLOT_SIZE - static_cast<i32>(textWidth) - 1,
                    bounds().y + DEFAULT_SLOT_SIZE - 8,
                    m_countColor);
            }
        }

        if (isHovered()) {
            ctx.drawBorder(bounds(), 1.0f, m_highlightColor);
        }
    }

    // ==================== 事件处理 ====================

    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)mods;

        if (!isActive() || !isVisible() || !m_interactive) return false;

        if (m_onSlotClick) {
            m_onSlotClick(m_slotIndex, button, m_shiftHeld);
        }

        return true;
    }

    bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;
        (void)mods;

        if (!isActive() || !isVisible() || !m_interactive) return false;

        if (m_onRelease) {
            m_onRelease(*this, button);
        }

        return true;
    }

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override
    {
        (void)key;
        (void)scanCode;

        const auto keyAction = static_cast<KeyAction>(action);
        if (keyAction == KeyAction::Press || keyAction == KeyAction::Repeat) {
            m_shiftHeld = hasMod(static_cast<KeyMods>(mods), KeyMods::Shift);
        }
        return false;
    }

    // ==================== 物品操作 ====================

    /**
     * @brief 设置物品
     */
    void setItem(const mc::ItemStack& item) { m_item = item; }

    /**
     * @brief 获取物品
     */
    [[nodiscard]] const mc::ItemStack& item() const { return m_item; }

    /**
     * @brief 获取可变物品引用
     */
    [[nodiscard]] mc::ItemStack& item() { return m_item; }

    /**
     * @brief 检查槽位是否为空
     */
    [[nodiscard]] bool isEmpty() const { return m_item.isEmpty(); }

    /**
     * @brief 清空槽位
     */
    void clearItem() { m_item = mc::ItemStack(); }

    // ==================== 属性设置 ====================

    /**
     * @brief 设置槽位索引
     */
    void setSlotIndex(i32 index) { m_slotIndex = index; }

    /**
     * @brief 获取槽位索引
     */
    [[nodiscard]] i32 slotIndex() const { return m_slotIndex; }

    /**
     * @brief 设置背景纹理路径
     */
    void setBackgroundTexture(const std::string& path) { m_backgroundTexture = path; }

    /**
     * @brief 获取背景纹理路径
     */
    [[nodiscard]] const std::string& backgroundTexture() const { return m_backgroundTexture; }

    /**
     * @brief 设置是否显示背景
     */
    void setShowBackground(bool show) { m_showBackground = show; }

    /**
     * @brief 是否显示背景
     */
    [[nodiscard]] bool showBackground() const { return m_showBackground; }

    /**
     * @brief 设置是否可交互
     */
    void setInteractive(bool interactive) { m_interactive = interactive; }

    /**
     * @brief 是否可交互
     */
    [[nodiscard]] bool isInteractive() const { return m_interactive; }

    /**
     * @brief 设置高亮颜色
     */
    void setHighlightColor(u32 color) { m_highlightColor = color; }

    /**
     * @brief 获取高亮颜色
     */
    [[nodiscard]] u32 highlightColor() const { return m_highlightColor; }

    /**
     * @brief 设置是否显示数量
     */
    void setShowCount(bool show) { m_showCount = show; }

    /**
     * @brief 是否显示数量
     */
    [[nodiscard]] bool showCount() const { return m_showCount; }

    // ==================== 渲染器注入（用于物品图标渲染） ====================

    /**
     * @brief 设置物品图标绘制回调
     *
     * 宿主屏幕注入一个调用 ItemRenderer::renderItem 的回调，使 SlotWidget 不直接依赖
     * ItemRenderer/GuiRenderer 类型。回调未注入时，槽位仅绘制背景/边框/数量。
     * @param callback 物品绘制回调
     */
    void setItemPaintCallback(ItemPaintCallback callback) { m_itemPaintCallback = std::move(callback); }

    /**
     * @brief 设置数量文本颜色（ARGB）
     */
    void setCountColor(u32 color) { m_countColor = color; }

    // ==================== 回调设置 ====================

    /**
     * @brief 设置槽位点击回调（与文档一致）
     * @param callback 回调函数，参数为(槽位索引, 鼠标按钮, 是否按住Shift)
     */
    void setOnSlotClick(OnSlotClickCallback callback) { m_onSlotClick = std::move(callback); }

    /**
     * @brief 设置释放回调
     */
    void setOnRelease(OnSlotReleaseCallback callback) { m_onRelease = std::move(callback); }

protected:
    mc::ItemStack m_item; ///< 槽位中的物品
    i32 m_slotIndex = -1; ///< 槽位索引

    // 显示属性
    std::string m_backgroundTexture; ///< 背景纹理路径
    bool m_showBackground = true;    ///< 是否显示背景
    bool m_interactive = true;       ///< 是否可交互
    bool m_showCount = true;         ///< 是否显示数量
    // TODO: m_shiftHeld通过键盘事件追踪Shift状态，但当Shift在组件外按下时可能不同步，
    //       后续应考虑从输入系统直接查询修饰键状态
    bool m_shiftHeld = false;                                    ///< Shift键是否按下
    u32 m_highlightColor = Colors::fromARGB(128, 255, 255, 255); ///< 高亮颜色
    u32 m_countColor = Colors::WHITE;                            ///< 数量文本颜色

    // 物品图标绘制回调（宿主注入，内部调用 ItemRenderer::renderItem，参考 HudWidget）
    ItemPaintCallback m_itemPaintCallback; ///< 物品绘制回调

    // 回调
    OnSlotClickCallback m_onSlotClick; ///< 槽位点击回调
    OnSlotReleaseCallback m_onRelease; ///< 释放回调
};

} // namespace mc::client::ui::kagero::widget
