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

#include "Screen.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/renderer/trident/gui/GuiTextureManager.hpp"
#include "client/renderer/trident/item/ItemRenderer.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/SlotWidget.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "core/Types.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc::client::ui::minecraft {

/**
 * @brief 容器屏共享基类（kagero 体系）
 *
 * 承载所有容器屏（背包/创造/工作台/箱子/熔炉/制图台）的共性：
 * - 居中定位（leftPos/topPos）与屏幕尺寸管理
 * - 槽位组件（SlotWidget）的构造、布局、命中检测、物品同步
 * - paint 模板方法：背景暗化 → 容器背景 → 槽位 → 前景 → carried item → tooltip
 * - 渲染器注入（GuiRenderer/GuiTextureManager/ItemRenderer）与物品图标回调下发
 *
 * 交互逻辑（点击/拖拽/QuickMove/PickupAll/Q丢弃/数字键交换）由子类持有的
 * ContainerInteraction<Menu> 处理，基类不介入。
 *
 * @tparam Menu 菜单类型（AbstractContainerMenu 子类），子类经 menu() 暴露。
 */
template <typename Menu>
class ContainerScreenBase : public Screen {
public:
    /**
     * @brief 构造函数
     * @param id 屏幕 ID
     * @param menu 容器菜单（子类转入）
     */
    ContainerScreenBase(std::string id, std::unique_ptr<Menu> menu)
        : Screen(std::move(id))
        , m_menu(std::move(menu))
    {}

    // ==================== 渲染器与尺寸 ====================

    /**
     * @brief 设置渲染器（与主调者统一签名）
     */
    void setRenderers(renderer::trident::gui::GuiRenderer* gui,
        renderer::trident::gui::GuiTextureManager* textureManager,
        renderer::trident::item::ItemRenderer* itemRenderer)
    {
        m_gui = gui;
        m_textureManager = textureManager;
        m_itemRenderer = itemRenderer;
        _injectSlotPaintCallbacks();
    }

    /**
     * @brief 设置屏幕尺寸并重新布局
     */
    void setScreenSize(i32 width, i32 height)
    {
        m_screenWidth = width;
        m_screenHeight = height;
        _relayout();
    }

    // ==================== 访问器 ====================

    [[nodiscard]] Menu* getMenu() const { return m_menu.get(); }
    [[nodiscard]] renderer::trident::gui::GuiRenderer* guiRenderer() const { return m_gui; }
    [[nodiscard]] renderer::trident::gui::GuiTextureManager* textureManager() const { return m_textureManager; }
    [[nodiscard]] renderer::trident::item::ItemRenderer* itemRenderer() const { return m_itemRenderer; }
    [[nodiscard]] i32 leftPos() const { return m_leftPos; }
    [[nodiscard]] i32 topPos() const { return m_topPos; }
    [[nodiscard]] i32 screenWidth() const { return m_screenWidth; }
    [[nodiscard]] i32 screenHeight() const { return m_screenHeight; }
    [[nodiscard]] i32 mouseX() const { return m_mouseX; }
    [[nodiscard]] i32 mouseY() const { return m_mouseY; }

    // ==================== 生命周期 ====================

    void onOpen() override
    {
        if (m_screenWidth > 0 && m_screenHeight > 0) {
            setBounds(kagero::Rect(0, 0, m_screenWidth, m_screenHeight));
        }
        _relayout();
        syncSlots();
    }

    void onResize(i32 width, i32 height) override
    {
        m_screenWidth = width;
        m_screenHeight = height;
        setBounds(kagero::Rect(0, 0, width, height));
        _relayout();
    }

    void paint(kagero::widget::PaintContext& ctx) override
    {
        if (!m_layoutDone) {
            _relayout();
        }

        // 1. 背景暗化（半透明黑）
        if (m_screenWidth > 0 && m_screenHeight > 0) {
            ctx.drawFilledRect(kagero::Rect(0, 0, m_screenWidth, m_screenHeight), Colors::fromARGB(128, 0, 0, 0));
        }

        // 2. 容器背景纹理（子类实现）
        renderContainerBackground(ctx);

        // 3. 额外子组件（创造屏的调色板/搜索框等，子类按需实现）
        renderExtraWidgets(ctx);

        // 4. 槽位组件
        renderSlots(ctx);

        // 5. 容器前景（标题等，子类实现）
        renderContainerForeground(ctx);

        // 6. 鼠标光标携带的物品
        renderCarriedItem(ctx);

        // 7. 悬停物品 tooltip
        renderTooltip(ctx);
    }

    void updateHover(i32 mouseX, i32 mouseY) override
    {
        m_mouseX = mouseX;
        m_mouseY = mouseY;
        Screen::updateHover(mouseX, mouseY);
    }

    // ==================== 槽位同步 ====================

    /**
     * @brief 从菜单同步槽位内容到 SlotWidget
     */
    void syncSlots()
    {
        if (m_menu == nullptr) {
            return;
        }
        const i32 count = std::min(m_menu->getSlotCount(), static_cast<i32>(m_slots.size()));
        for (i32 i = 0; i < count; ++i) {
            if (const mc::Slot* slot = m_menu->getSlot(i)) {
                m_slots[i]->setItem(slot->getItem());
            }
        }
    }

    /**
     * @brief 根据屏幕坐标查找命中的槽位（供子类交互引擎使用）
     */
    [[nodiscard]] mc::Slot* slotAt(i32 mouseX, i32 mouseY)
    {
        if (m_menu == nullptr) {
            return nullptr;
        }
        for (i32 i = 0; i < m_menu->getSlotCount() && i < static_cast<i32>(m_slots.size()); ++i) {
            if (m_slots[i]->contains(mouseX, mouseY)) {
                return m_menu->getSlot(i);
            }
        }
        return nullptr;
    }

protected:
    // ========== 布局常量（所有容器屏共享）==========
    static constexpr i32 SLOT_SIZE = 16;
    static constexpr i32 SLOT_SPACING = 18;

    // 玩家背包区在 GUI 内的相对坐标（背包/创造/工作台等通用）
    static constexpr i32 ARMOR_X = 8;
    static constexpr i32 ARMOR_Y_HEAD = 8;
    static constexpr i32 ARMOR_Y_CHEST = 26;
    static constexpr i32 ARMOR_Y_LEGS = 44;
    static constexpr i32 ARMOR_Y_FEET = 62;
    static constexpr i32 OFFHAND_X = 77;
    static constexpr i32 OFFHAND_Y = 62;
    static constexpr i32 PLAYER_INV_X = 8;
    static constexpr i32 PLAYER_INV_Y = 84;
    static constexpr i32 HOTBAR_X = 8;
    static constexpr i32 HOTBAR_Y = 142;

    // ========== 子类必须实现的钩子 ==========

    /** @brief GUI 宽度（像素） */
    [[nodiscard]] virtual i32 guiWidth() const = 0;
    /** @brief GUI 高度（像素） */
    [[nodiscard]] virtual i32 guiHeight() const = 0;
    /** @brief 某槽位在 GUI 内的相对坐标 */
    [[nodiscard]] virtual std::pair<i32, i32> slotLocalPos(i32 slotIndex) const = 0;
    /** @brief 鼠标光标携带的物品（空堆若无） */
    [[nodiscard]] virtual const mc::ItemStack& getCarriedItem() const = 0;

    /** @brief 绘制容器背景纹理（不含暗化层，暗化由基类 paint 已画） */
    virtual void renderContainerBackground(kagero::widget::PaintContext& ctx) = 0;
    /**
     * @brief 绘制额外子组件（在容器背景之后、槽位之前）
     *
     * 默认空实现。创造屏等带非槽位子组件（调色板网格/搜索框）的屏幕重写此方法，
     * 直接调用子组件的 paint。基类 renderSlots 只绘制 SlotWidget，不绘制其他子组件。
     */
    virtual void renderExtraWidgets(kagero::widget::PaintContext& ctx) { (void)ctx; }
    /** @brief 绘制容器前景（标题等） */
    virtual void renderContainerForeground(kagero::widget::PaintContext& ctx) = 0;
    /** @brief 绘制鼠标光标携带的物品（默认实现：跟随鼠标画图标+数量） */
    virtual void renderCarriedItem(kagero::widget::PaintContext& ctx)
    {
        const mc::ItemStack& carried = getCarriedItem();
        if (carried.isEmpty() || m_itemRenderer == nullptr || m_gui == nullptr) {
            return;
        }
        m_itemRenderer->renderItem(*m_gui,
            carried,
            static_cast<f64>(m_mouseX - SLOT_SIZE / 2),
            static_cast<f64>(m_mouseY - SLOT_SIZE / 2),
            static_cast<f64>(SLOT_SIZE));
        if (carried.getCount() > 1) {
            const std::string countText = std::to_string(carried.getCount());
            const f32 textWidth = ctx.getTextWidth(countText);
            ctx.drawText(countText,
                m_mouseX + SLOT_SIZE / 2 - static_cast<i32>(textWidth) - 1,
                m_mouseY + SLOT_SIZE / 2 - 8,
                Colors::WHITE);
        }
    }
    /** @brief 绘制悬停物品 tooltip（默认空，子类按需实现） */
    virtual void renderTooltip(kagero::widget::PaintContext& ctx) { (void)ctx; }

    // ========== 槽位组件管理 ==========

    /**
     * @brief 构造指定数量的槽位组件并加入子节点
     * @param slotCount 槽位数量
     */
    void buildSlots(i32 slotCount)
    {
        m_slots.clear();
        m_slots.reserve(slotCount);
        for (i32 i = 0; i < slotCount; ++i) {
            auto slot = std::make_unique<kagero::widget::SlotWidget>("slot_" + std::to_string(i), 0, 0, SLOT_SIZE);
            slot->setSlotIndex(i);
            slot->setShowBackground(false);
            m_slots.push_back(slot.get());
            addChild(std::move(slot));
        }
        _injectSlotPaintCallbacks();
    }

    [[nodiscard]] const std::vector<kagero::widget::SlotWidget*>& slots() const { return m_slots; }

    // ========== 数据成员 ==========
    std::unique_ptr<Menu> m_menu;

    renderer::trident::gui::GuiRenderer* m_gui = nullptr;
    renderer::trident::gui::GuiTextureManager* m_textureManager = nullptr;
    renderer::trident::item::ItemRenderer* m_itemRenderer = nullptr;

    std::vector<kagero::widget::SlotWidget*> m_slots;

    i32 m_leftPos = 0;
    i32 m_topPos = 0;
    i32 m_screenWidth = 0;
    i32 m_screenHeight = 0;
    i32 m_mouseX = 0;
    i32 m_mouseY = 0;
    bool m_layoutDone = false;

    // ========== 内部实现 ==========

    void renderSlots(kagero::widget::PaintContext& ctx)
    {
        for (auto* slot : m_slots) {
            if (slot->isVisible()) {
                slot->paint(ctx);
            }
        }
    }

    void _relayout()
    {
        m_leftPos = (m_screenWidth > 0) ? (m_screenWidth - guiWidth()) / 2 : 0;
        m_topPos = (m_screenHeight > 0) ? (m_screenHeight - guiHeight()) / 2 : 0;

        for (i32 i = 0; i < static_cast<i32>(m_slots.size()); ++i) {
            auto [lx, ly] = slotLocalPos(i);
            m_slots[i]->setBounds(kagero::Rect(m_leftPos + lx, m_topPos + ly, SLOT_SIZE, SLOT_SIZE));
        }
        m_layoutDone = true;
    }

    void _injectSlotPaintCallbacks()
    {
        if (m_gui == nullptr || m_itemRenderer == nullptr) {
            return;
        }
        auto* guiPtr = m_gui;
        auto* itemRendererPtr = m_itemRenderer;
        for (auto* slot : m_slots) {
            slot->setItemPaintCallback([guiPtr, itemRendererPtr](const mc::ItemStack& item, i32 x, i32 y, i32 size) {
                itemRendererPtr->renderItem(
                    *guiPtr, item, static_cast<f64>(x), static_cast<f64>(y), static_cast<f64>(size));
            });
        }
    }
};

} // namespace mc::client::ui::minecraft
