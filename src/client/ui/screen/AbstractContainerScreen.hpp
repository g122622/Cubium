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

#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/ui/screen/tooltip/BundleTooltipRenderer.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/items/special/bundle/BundleItem.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/screen/IScreen.hpp"
#include "common/world/IWorld.hpp"
#include "core/Types.hpp"
#include "entity/inventory/AbstractContainerMenu.hpp"
#include "entity/inventory/ContainerTypes.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <vector>
#include <GLFW/glfw3.h>

namespace mc::client::renderer::trident::gui {
class GuiRenderer;
class GuiTextureManager;
} // namespace mc::client::renderer::trident::gui

namespace mc::client::renderer::trident::item {
class ItemRenderer;
}

namespace mc::client {

/**
 * @brief 容器屏幕基类
 *
 * 管理容器菜单的客户端屏幕，处理槽位渲染和交互。
 * 与服务端的AbstractContainerMenu配对使用。
 *
 * 交互类型：
 * - 左键点击：拾取/放置物品
 * - 右键点击：拾取半组/放置一个物品
 * - Shift+左键：快速移动物品到对应区域
 * - Shift+双击：快速移动所有匹配物品
 * - 数字键1-9：与快捷栏槽位交换
 * - F键：与副手槽位交换
 * - Q键：丢弃一个物品，Ctrl+Q丢弃整组
 * - 中键（创造模式）：复制物品
 * - 左键拖拽：均匀分发物品
 * - 右键拖拽：逐个分发物品
 * - 中键拖拽（创造模式）：填满槽位
 * - 双击：拾取所有相同物品
 * - 点击屏幕外：丢弃光标上的物品
 *
 * @tparam Menu 菜单类型
 */
template <typename Menu>
class AbstractContainerScreen : public IScreen {
public:
    using ContainerClickSender = std::function<void(ContainerId, i32, i32, i16, ClickAction, const mc::ItemStack&)>;
    using ContainerCloseSender = std::function<void(ContainerId)>;

    /**
     * @brief 构造函数
     * @param menu 菜单实例
     * @param clickSender 容器点击事件发送器
     * @param closeSender 容器关闭事件发送器
     */
    explicit AbstractContainerScreen(
        std::unique_ptr<Menu> menu, ContainerClickSender clickSender, ContainerCloseSender closeSender)
        : m_menu(std::move(menu))
        , m_clickSender(std::move(clickSender))
        , m_closeSender(std::move(closeSender))
    {}

    /**
     * @brief 析构函数
     */
    ~AbstractContainerScreen() override = default;

    // 禁止拷贝
    AbstractContainerScreen(const AbstractContainerScreen&) = delete;
    AbstractContainerScreen& operator=(const AbstractContainerScreen&) = delete;

    // 允许移动
    AbstractContainerScreen(AbstractContainerScreen&&) noexcept = default;
    AbstractContainerScreen& operator=(AbstractContainerScreen&&) noexcept = default;

    /**
     * @brief 设置渲染器
     * @param gui GUI渲染器
     * @param textureManager GUI纹理管理器
     * @param itemRenderer 物品渲染器
     */
    void setRenderers(renderer::trident::gui::GuiRenderer* gui,
        renderer::trident::gui::GuiTextureManager* textureManager,
        renderer::trident::item::ItemRenderer* itemRenderer)
    {
        m_gui = gui;
        m_textureManager = textureManager;
        m_itemRenderer = itemRenderer;
    }

    /**
     * @brief 设置屏幕尺寸
     * @param width 屏幕宽度
     * @param height 屏幕高度
     */
    void setScreenSize(i32 width, i32 height)
    {
        m_screenWidth = width;
        m_screenHeight = height;
        updatePosition();
    }

    /**
     * @brief 初始化屏幕
     */
    void init() override
    {
        if (!m_initialized) {
            m_initialized = true;
            onInit();
        }
    }

    /**
     * @brief 渲染屏幕
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param partialTick 部分tick时间
     */
    void render(i32 mouseX, i32 mouseY, f32 partialTick) override
    {
        (void)partialTick;

        if (m_gui == nullptr) {
            return;
        }

        // 更新悬停槽位索引（用于键盘操作）
        _updateHoveredSlot(mouseX, mouseY);

        // 开始GUI帧
        m_gui->beginFrame(static_cast<f32>(m_screenWidth), static_cast<f32>(m_screenHeight));

        // 渲染背景
        if (shouldRenderBackground()) {
            renderBackground();
        }

        // 渲染容器GUI
        renderContainerBackground();
        renderSlots();
        renderContainerForeground(mouseX, mouseY);

        // 渲染鼠标持有的物品
        renderCarriedItem(mouseX, mouseY);

        // 渲染悬停提示
        renderTooltip(mouseX, mouseY);
    }

    /**
     * @brief 处理鼠标点击
     *
     * 根据 Shift/Ctrl 修饰键和鼠标按钮，决定交互类型：
     * - 左键(0): 拾取/放置
     * - 右键(1): 拾取半组/放置一个
     * - 中键(2): 创造模式复制
     * - Shift+左键: 快速移动
     * - 如果光标持有物品且不在拖拽中: 开始拖拽分发
     *
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param button 鼠标按键
     * @param mods 修饰键位掩码
     * @return 是否处理了点击事件
     */
    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        if (m_menu == nullptr) {
            return false;
        }

        const bool shiftHeld = (mods & GLFW_MOD_SHIFT) != 0;
        const bool ctrlHeld = (mods & GLFW_MOD_CONTROL) != 0;

        // 查找点击的槽位
        mc::Slot* slot = getSlotAt(mouseX, mouseY);
        const bool clickedOutside = (slot == nullptr);

        // 更新悬停槽位索引
        m_hoveredSlotIndex = clickedOutside ? -1 : slot->getIndex();

        // 如果光标持有物品且不在拖拽中，且点击了有效槽位，进入拖拽模式
        const bool carriedHasItems = !getCarriedItem().isEmpty();
        if (carriedHasItems && !m_isQuickCrafting && !clickedOutside && button <= 1) {
            m_isQuickCrafting = true;
            m_quickCraftingButton = button;
            m_quickCraftingType = _getQuickCraftType(button, ctrlHeld);
            m_quickCraftSlots.clear();
            m_skipNextRelease = false;
            return true;
        }

        if (clickedOutside) {
            // 点击屏幕外部（槽位索引 -999）
            return _handleClickOutside(button, shiftHeld);
        }

        // 检测双击
        const auto now = std::chrono::steady_clock::now();
        const bool isDoubleClick =
            (m_lastClickSlot == slot && m_lastClickTime > std::chrono::steady_clock::time_point{} &&
                std::chrono::duration_cast<std::chrono::nanoseconds>(now - m_lastClickTime).count() <
                    DOUBLE_CLICK_THRESHOLD_NS);
        m_lastClickSlot = slot;
        m_lastClickTime = now;

        if (isDoubleClick && button == 0 && !shiftHeld) {
            // 双击：拾取所有相同物品
            _sendSlotClick(*slot, slot->getIndex(), 0, ClickAction::PickupAll);
            m_skipNextRelease = true;
            return true;
        }

        // Shift+左键：快速移动
        if (shiftHeld && button == 0) {
            _sendSlotClick(*slot, slot->getIndex(), 0, ClickAction::QuickMove);
            m_skipNextRelease = true;
            return true;
        }

        // 中键：创造模式复制
        if (button == 2) {
            _sendSlotClick(*slot, slot->getIndex(), 2, ClickAction::Clone);
            m_skipNextRelease = true;
            return true;
        }

        // 左键或右键：拾取/放置
        if (button == 0 || button == 1) {
            _sendSlotClick(*slot, slot->getIndex(), button, ClickAction::Pickup);
            m_skipNextRelease = true;
            return true;
        }

        return onSlotClick(*slot, slot->getIndex(), button);
    }

    /**
     * @brief 处理鼠标释放
     *
     * 如果正在拖拽分发，完成拖拽操作。
     * 否则忽略释放事件（点击已在 onClick 中处理）。
     */
    bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods) override
    {
        (void)mouseX;
        (void)mouseY;

        if (m_menu == nullptr) {
            return false;
        }

        // 如果设置了跳过下一次释放，清除标记并忽略
        if (m_skipNextRelease) {
            m_skipNextRelease = false;
            return true;
        }

        // 处理拖拽分发完成
        if (m_isQuickCrafting && button == m_quickCraftingButton) {
            const bool shiftHeld = (mods & GLFW_MOD_SHIFT) != 0;
            _finishQuickCraft(shiftHeld);
            return true;
        }

        return false;
    }

    /**
     * @brief 处理鼠标拖动
     *
     * 如果正在拖拽分发，将鼠标下的有效槽位添加到拖拽目标列表中。
     */
    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button) override
    {
        (void)deltaX;
        (void)deltaY;

        if (m_menu == nullptr || !m_isQuickCrafting || button != m_quickCraftingButton) {
            return false;
        }

        // 查找鼠标下的槽位
        mc::Slot* slot = getSlotAt(mouseX, mouseY);
        if (slot == nullptr) {
            return true;
        }

        // 更新悬停槽位索引
        m_hoveredSlotIndex = slot->getIndex();

        // 检查槽位是否可以接受拖拽物品
        const ItemStack& carried = getCarriedItem();
        if (carried.isEmpty()) {
            return true;
        }

        // 检查槽位是否已经在拖拽列表中
        const i32 slotIndex = slot->getIndex();
        for (i32 idx : m_quickCraftSlots) {
            if (idx == slotIndex) {
                return true; // 已在列表中
            }
        }

        // 检查是否可以放入物品
        if (!slot->mayPlace(carried)) {
            return true;
        }

        // 检查数量限制（均匀/逐个模式需要数量大于已选槽位数）
        if (m_quickCraftingType != DragConstants::MODE_FILL) {
            if (carried.getCount() <= static_cast<i32>(m_quickCraftSlots.size())) {
                return true;
            }
        }

        // 检查槽位物品是否可以合并
        if (!slot->getItem().isEmpty() && !carried.isSameItem(slot->getItem())) {
            return true;
        }

        m_quickCraftSlots.push_back(slotIndex);
        return true;
    }

    /**
     * @brief 处理键盘按键
     *
     * 处理以下键盘操作：
     * - ESC/E: 关闭屏幕
     * - 数字键1-9: 与快捷栏槽位交换
     * - F: 与副手槽位交换
     * - Q: 丢弃一个物品，Ctrl+Q丢弃整组
     */
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override
    {
        (void)scanCode;

        if (m_menu == nullptr) {
            return false;
        }

        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            // ESC关闭屏幕
            if (key == GLFW_KEY_ESCAPE) {
                onClose();
                return true;
            }

            // E键关闭容器屏幕
            if (key == GLFW_KEY_E) {
                onClose();
                return true;
            }

            // Q键丢弃物品
            if (key == GLFW_KEY_Q) {
                return _handleDropKey(mods);
            }

            // 数字键1-9交换快捷栏
            if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
                const i32 hotbarIndex = key - GLFW_KEY_1;
                return _handleHotbarSwap(hotbarIndex);
            }

            // F键交换副手
            if (key == GLFW_KEY_F) {
                return _handleHotbarSwap(40); // 40 = 副手槽位索引
            }
        }

        return false;
    }

    /**
     * @brief 屏幕关闭
     */
    void onClose() override
    {
        if (m_closeSender && m_menu != nullptr && m_menu->getId() != mc::inventory::PLAYER_CONTAINER_ID) {
            m_closeSender(m_menu->getId());
        }
    }

    /**
     * @brief 容器屏幕不暂停游戏
     */
    [[nodiscard]] bool isPauseScreen() const override { return false; }

    /**
     * @brief 获取菜单
     */
    [[nodiscard]] Menu* getMenu() { return m_menu.get(); }
    [[nodiscard]] const Menu* getMenu() const { return m_menu.get(); }

    /**
     * @brief 获取鼠标持有的物品
     */
    [[nodiscard]] mc::ItemStack& getCarriedItem() { return m_menu ? m_menu->getCarriedItem() : s_emptyStack; }
    [[nodiscard]] const mc::ItemStack& getCarriedItem() const
    {
        return m_menu ? m_menu->getCarriedItem() : s_emptyStack;
    }

    /**
     * @brief 获取当前悬停的槽位索引
     * @return 悬停槽位索引，如果没有悬停槽位返回-1
     */
    [[nodiscard]] i32 getHoveredSlotIndex() const { return m_hoveredSlotIndex; }

    /**
     * @brief 获取GUI左边界
     */
    [[nodiscard]] i32 getLeftPos() const { return m_leftPos; }

    /**
     * @brief 获取GUI上边界
     */
    [[nodiscard]] i32 getTopPos() const { return m_topPos; }

    /**
     * @brief 获取GUI宽度
     */
    [[nodiscard]] i32 getImageWidth() const { return m_imageWidth; }

    /**
     * @brief 获取GUI高度
     */
    [[nodiscard]] i32 getImageHeight() const { return m_imageHeight; }

    /**
     * @brief 获取GUI渲染器
     */
    [[nodiscard]] renderer::trident::gui::GuiRenderer* getGuiRenderer() { return m_gui; }
    [[nodiscard]] const renderer::trident::gui::GuiRenderer* getGuiRenderer() const { return m_gui; }

protected:
    // 槽位尺寸常量
    static constexpr i32 SLOT_SIZE = 16;
    static constexpr i32 SLOT_SPACING = 18;

    // 双击检测阈值（纳秒，500ms）
    static constexpr i64 DOUBLE_CLICK_THRESHOLD_NS = 500'000'000L;

    /**
     * @brief 子类初始化回调
     */
    virtual void onInit() {}

    /**
     * @brief 设置GUI尺寸
     * @param width 宽度
     * @param height 高度
     */
    void setImageSize(i32 width, i32 height)
    {
        m_imageWidth = width;
        m_imageHeight = height;
        updatePosition();
    }

    /**
     * @brief 更新GUI位置（居中）
     */
    void updatePosition()
    {
        // 居中计算
        if (m_screenWidth > 0 && m_screenHeight > 0) {
            m_leftPos = (m_screenWidth - m_imageWidth) / 2;
            m_topPos = (m_screenHeight - m_imageHeight) / 2;
        } else {
            m_leftPos = 0;
            m_topPos = 0;
        }
    }

    /**
     * @brief 渲染背景暗化
     */
    virtual void renderBackground()
    {
        // 半透明黑色背景 (ARGB)
        m_gui->fillRect(0.0f, 0.0f, static_cast<f32>(m_screenWidth), static_cast<f32>(m_screenHeight), 0x80000000);
    }

    /**
     * @brief 渲染容器背景（子类重写以渲染纹理背景）
     */
    virtual void renderContainerBackground()
    {
        // 子类实现具体的背景渲染
    }

    /**
     * @brief 渲染所有槽位
     */
    virtual void renderSlots()
    {
        if (m_menu == nullptr || m_gui == nullptr) {
            return;
        }

        for (i32 i = 0; i < m_menu->getSlotCount(); ++i) {
            const mc::Slot* slot = m_menu->getSlot(i);
            if (slot != nullptr) {
                renderSlot(*slot, m_leftPos + slot->getX(), m_topPos + slot->getY());
            }
        }
    }

    /**
     * @brief 渲染单个槽位
     * @param slot 槽位
     * @param screenX 屏幕X坐标
     * @param screenY 屏幕Y坐标
     */
    virtual void renderSlot(const mc::Slot& slot, i32 screenX, i32 screenY)
    {
        // 渲染物品
        const mc::ItemStack& stack = slot.getItem();
        if (!stack.isEmpty()) {
            renderItemIcon(stack, screenX, screenY);

            // 渲染物品数量
            if (stack.getCount() > 1) {
                renderItemCount(stack.getCount(), screenX + SLOT_SIZE - 2, screenY + SLOT_SIZE - 8);
            }
        }
    }

    /**
     * @brief 渲染物品图标（子类可重写以使用ItemRenderer）
     * @param stack 物品堆
     * @param screenX 屏幕X坐标
     * @param screenY 屏幕Y坐标
     */
    virtual void renderItemIcon(const mc::ItemStack& stack, i32 screenX, i32 screenY)
    {
        // 默认实现：绘制占位符矩形
        // 子类可以重写此方法，使用 ItemRenderer 渲染实际的物品图标
        (void)stack;
        m_gui->fillRect(static_cast<f32>(screenX),
            static_cast<f32>(screenY),
            static_cast<f32>(SLOT_SIZE),
            static_cast<f32>(SLOT_SIZE),
            0x80FFFFFF);
    }

    /**
     * @brief 渲染物品数量文字
     * @param count 数量
     * @param screenX 屏幕X坐标
     * @param screenY 屏幕Y坐标
     */
    void renderItemCount(i32 count, i32 screenX, i32 screenY)
    {
        if (m_gui->font() == nullptr || count <= 1) {
            return;
        }

        std::string countText = std::to_string(count);
        m_gui->drawText(countText, static_cast<f32>(screenX), static_cast<f32>(screenY), 0xFFFFFFFF, true);
    }

    /**
     * @brief 渲染槽位高亮
     * @param screenX 屏幕X坐标
     * @param screenY 屏幕Y坐标
     */
    void renderSlotHighlight(i32 screenX, i32 screenY)
    {
        // 槽位高亮颜色 (ARGB，半透明白色)
        m_gui->fillRect(static_cast<f32>(screenX),
            static_cast<f32>(screenY),
            static_cast<f32>(SLOT_SIZE),
            static_cast<f32>(SLOT_SIZE),
            0x40FFFFFF);
    }

    /**
     * @brief 渲染容器前景（标题等）
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     */
    virtual void renderContainerForeground(i32 mouseX, i32 mouseY)
    {
        (void)mouseX;
        (void)mouseY;
        // 子类可重写以渲染标题
    }

    /**
     * @brief 渲染鼠标持有的物品
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     */
    virtual void renderCarriedItem(i32 mouseX, i32 mouseY)
    {
        const auto& carried = getCarriedItem();
        if (carried.isEmpty()) {
            return;
        }

        // 渲染跟随鼠标的物品
        renderItemIcon(carried, mouseX - SLOT_SIZE / 2, mouseY - SLOT_SIZE / 2);

        // 渲染物品数量
        if (carried.getCount() > 1) {
            renderItemCount(carried.getCount(), mouseX + SLOT_SIZE / 2 - 2, mouseY + SLOT_SIZE / 2 - 8);
        }
    }

    /**
     * @brief 渲染悬停提示
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     */
    virtual void renderTooltip(i32 mouseX, i32 mouseY)
    {
        mc::Slot* slot = getSlotAt(mouseX, mouseY);
        if (slot != nullptr) {
            renderItemTooltip(slot->getItem(), mouseX, mouseY);
        }
    }

    /**
     * @brief 渲染物品提示框
     *
     * 渲染流程：
     * 1. 收纳袋物品：委托给 BundleTooltipRenderer 渲染网格 + 进度条
     * 2. 普通物品：渲染物品名 + 数量 + 耐久 + Item::addInformation 自定义行
     *
     * @param stack 物品堆
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     */
    void renderItemTooltip(const mc::ItemStack& stack, i32 mouseX, i32 mouseY)
    {
        if (m_gui->font() == nullptr || stack.isEmpty()) {
            return;
        }

        // 收纳袋物品：使用专用 tooltip 渲染器
        // 对应 MC 1.21.11 中 BundleItem 通过 getTooltipImage 返回 BundleTooltip
        if (mc::item::items::BundleItem::isBundleItem(stack) && m_itemRenderer != nullptr) {
            mc::client::ui::screen::tooltip::BundleTooltipRenderer::render(*m_gui,
                *m_itemRenderer,
                stack,
                mouseX,
                mouseY,
                m_screenWidth,
                m_screenHeight,
                mc::client::ui::screen::tooltip::BundleTooltipRenderer::BORDER_COLOR);
            return;
        }

        std::vector<std::string> lines;
        auto displayName = stack.getDisplayName();
        lines.emplace_back(displayName ? displayName->getUnformattedText() : "");

        if (stack.getCount() > 1) {
            lines.emplace_back("Count: " + std::to_string(stack.getCount()));
        }

        if (stack.isDamageable() && stack.getMaxDamage() > 0) {
            const i32 remainingDurability = std::max(0, stack.getMaxDamage() - stack.getDamage());
            lines.emplace_back(
                "Durability: " + std::to_string(remainingDurability) + "/" + std::to_string(stack.getMaxDamage()));
        }

        // 调用 Item::addInformation 附加物品自定义 tooltip
        // 对应 MC 1.21.11 ItemStack#getTooltipLines 调用 Item#appendHoverText
        // MC 中 TooltipContext.of(level) 在 level 为 null 时返回 EMPTY 上下文，
        // 本项目用 IWorld* 表达同样的可空语义：客户端 Player 的 world() 为 null
        // （ClientWorld 不继承 IWorld），此时传 nullptr，子类按需跳过依赖世界的逻辑。
        if (stack.getItem() != nullptr) {
            mc::IWorld* world = nullptr;
            if (m_menu != nullptr) {
                auto* playerInventory = m_menu->getPlayerInventory();
                if (playerInventory != nullptr && playerInventory->getPlayer() != nullptr) {
                    world = playerInventory->getPlayer()->world();
                }
            }
            stack.getItem()->addInformation(stack, world, lines, false);
        }

        f64 maxTextWidth = 0.0;
        for (const auto& line : lines) {
            maxTextWidth = std::max(maxTextWidth, m_gui->getTextWidth(line));
        }

        constexpr f64 PADDING = 4.0;
        constexpr f64 MARGIN = 12.0;
        const f64 fontHeight = static_cast<f64>(m_gui->getFontHeight());
        const f64 tooltipWidth = maxTextWidth + PADDING * 2.0;
        const f64 tooltipHeight = static_cast<f64>(lines.size()) * fontHeight + PADDING * 2.0;

        f64 tooltipX = static_cast<f64>(mouseX) + MARGIN;
        f64 tooltipY = static_cast<f64>(mouseY) + MARGIN;
        const f64 screenWidth = static_cast<f64>(m_screenWidth);
        const f64 screenHeight = static_cast<f64>(m_screenHeight);

        if (tooltipX + tooltipWidth > screenWidth) {
            tooltipX = static_cast<f64>(mouseX) - MARGIN - tooltipWidth;
        }
        if (tooltipY + tooltipHeight > screenHeight) {
            tooltipY = static_cast<f64>(mouseY) - MARGIN - tooltipHeight;
        }

        tooltipX = std::max(4.0, tooltipX);
        tooltipY = std::max(4.0, tooltipY);

        m_gui->fillRect(tooltipX, tooltipY, tooltipWidth, tooltipHeight, 0xF0100010);
        m_gui->drawRect(tooltipX, tooltipY, tooltipWidth, tooltipHeight, 0x505000FF);

        const f64 textX = tooltipX + PADDING;
        f64 textY = tooltipY + PADDING;
        for (const auto& line : lines) {
            m_gui->drawText(line, textX, textY, 0xFFFFFFFF, true);
            textY += fontHeight;
        }
    }

    /**
     * @brief 获取指定位置的槽位
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @return 槽位指针，如果无效返回nullptr
     */
    [[nodiscard]] mc::Slot* getSlotAt(i32 mouseX, i32 mouseY)
    {
        if (m_menu == nullptr) {
            return nullptr;
        }

        // 遍历所有槽位查找
        for (i32 i = 0; i < m_menu->getSlotCount(); ++i) {
            mc::Slot* slot = m_menu->getSlot(i);
            if (slot != nullptr && isMouseOverSlot(*slot, mouseX, mouseY)) {
                return slot;
            }
        }
        return nullptr;
    }

    /**
     * @brief 检查鼠标是否在槽位上
     * @param slot 槽位
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @return 鼠标是否在槽位区域内
     */
    [[nodiscard]] virtual bool isMouseOverSlot(const mc::Slot& slot, i32 mouseX, i32 mouseY) const
    {
        i32 slotX = m_leftPos + slot.getX();
        i32 slotY = m_topPos + slot.getY();
        return mouseX >= slotX && mouseX < slotX + SLOT_SIZE && mouseY >= slotY && mouseY < slotY + SLOT_SIZE;
    }

    /**
     * @brief 槽位点击处理（子类可重写以添加特殊逻辑）
     * @param slot 点击的槽位
     * @param slotIndex 槽位索引
     * @param button 鼠标按键
     * @return 是否处理了点击事件
     *
     * 子类可以重写此方法来实现特殊的槽位点击逻辑（如合成结果槽位）。
     * 默认实现通过 _sendSlotClick 将点击事件发送到菜单。
     */
    virtual bool onSlotClick(mc::Slot& slot, i32 slotIndex, i32 button)
    {
        (void)slot;
        (void)button;

        if (m_menu == nullptr) {
            return false;
        }

        // 默认：左键拾取/放置
        _sendSlotClick(slot, slotIndex, button, ClickAction::Pickup);
        return true;
    }

    /**
     * @brief 点击空白区域处理
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param button 鼠标按键
     * @return 是否处理了点击事件
     */
    virtual bool onClickOutside(i32 mouseX, i32 mouseY, i32 button)
    {
        (void)mouseX;
        (void)mouseY;
        (void)button;
        return false;
    }

    // 菜单与事件发送器
    std::unique_ptr<Menu> m_menu;
    ContainerClickSender m_clickSender;
    ContainerCloseSender m_closeSender;

    // 渲染器
    renderer::trident::gui::GuiRenderer* m_gui = nullptr;
    renderer::trident::gui::GuiTextureManager* m_textureManager = nullptr;
    renderer::trident::item::ItemRenderer* m_itemRenderer = nullptr;

    i32 m_leftPos = 0;          ///< GUI左边界（居中后的位置）
    i32 m_topPos = 0;           ///< GUI上边界
    i32 m_imageWidth = 176;     ///< GUI纹理宽度
    i32 m_imageHeight = 166;    ///< GUI纹理高度
    i32 m_screenWidth = 0;      ///< 屏幕宽度
    i32 m_screenHeight = 0;     ///< 屏幕高度
    bool m_initialized = false; ///< 是否已初始化

    // 空物品堆（用于空菜单时返回）
    static mc::ItemStack s_emptyStack;

private:
    /**
     * @brief 更新悬停槽位索引
     *
     * 在每帧渲染时调用，跟踪当前鼠标悬停的槽位索引，
     * 用于键盘操作（Q键丢弃、数字键交换等）。
     */
    void _updateHoveredSlot(i32 mouseX, i32 mouseY)
    {
        mc::Slot* slot = getSlotAt(mouseX, mouseY);
        m_hoveredSlotIndex = (slot != nullptr) ? slot->getIndex() : -1;
    }

    /**
     * @brief 发送槽位点击事件
     *
     * 根据是否有 clickSender 决定是网络模式还是本地模式。
     * 网络模式：发送 ContainerClickPacket 到服务端。
     * 本地模式：直接调用菜单的 clicked 方法。
     *
     * @param slot 点击的槽位
     * @param slotIndex 槽位索引
     * @param button 鼠标按钮
     * @param action 点击操作类型
     */
    void _sendSlotClick(mc::Slot& slot, i32 slotIndex, i32 button, ClickAction action)
    {
        if (m_clickSender) {
            const i16 transactionId = m_menu->incrementTransactionId();
            m_clickSender(m_menu->getId(), slotIndex, button, transactionId, action, m_menu->getCarriedItem());
            return;
        }

        // 本地模式：将 ClickAction 转换为 ClickType
        auto* playerInventory = m_menu->getPlayerInventory();
        if (playerInventory == nullptr || playerInventory->getPlayer() == nullptr) {
            return;
        }

        ClickType clickType = _actionToClickType(action, button);
        m_menu->clicked(slotIndex, button, clickType, *playerInventory->getPlayer());
    }

    /**
     * @brief 发送特殊槽位点击事件（使用 -999 槽位索引）
     * @param button 鼠标按钮
     * @param action 点击操作类型
     */
    void _sendOutsideClick(i32 button, ClickAction action)
    {
        if (m_clickSender) {
            const i16 transactionId = m_menu->incrementTransactionId();
            m_clickSender(
                m_menu->getId(), SLOT_CLICKED_OUTSIDE, button, transactionId, action, m_menu->getCarriedItem());
            return;
        }

        auto* playerInventory = m_menu->getPlayerInventory();
        if (playerInventory == nullptr || playerInventory->getPlayer() == nullptr) {
            return;
        }

        ClickType clickType = _actionToClickType(action, button);
        m_menu->clicked(SLOT_CLICKED_OUTSIDE, button, clickType, *playerInventory->getPlayer());
    }

    /**
     * @brief 处理点击屏幕外部（丢弃光标物品）
     */
    bool _handleClickOutside(i32 button, bool shiftHeld)
    {
        if (m_menu == nullptr || getCarriedItem().isEmpty()) {
            return false;
        }

        // 点击外部丢弃光标物品
        if (button == 0) {
            // 左键：丢弃全部
            _sendOutsideClick(0, ClickAction::Pickup);
            return true;
        }
        if (button == 1) {
            // 右键：丢弃一个
            _sendOutsideClick(1, ClickAction::Pickup);
            return true;
        }

        (void)shiftHeld;
        return false;
    }

    /**
     * @brief 处理Q键丢弃物品
     * @param mods 修饰键位掩码
     * @return 是否处理了事件
     */
    bool _handleDropKey(i32 mods)
    {
        if (m_menu == nullptr) {
            return false;
        }

        // 如果光标持有物品，丢弃光标物品
        if (!getCarriedItem().isEmpty()) {
            const bool ctrlHeld = (mods & GLFW_MOD_CONTROL) != 0;
            _sendOutsideClick(ctrlHeld ? 1 : 0, ClickAction::Throw);
            return true;
        }

        // 否则丢弃鼠标悬停槽位中的物品
        if (m_hoveredSlotIndex >= 0 && m_hoveredSlotIndex < m_menu->getSlotCount()) {
            const bool ctrlHeld = (mods & GLFW_MOD_CONTROL) != 0;
            mc::Slot* slot = m_menu->getSlot(m_hoveredSlotIndex);
            if (slot != nullptr && !slot->getItem().isEmpty()) {
                _sendSlotClick(*slot, m_hoveredSlotIndex, ctrlHeld ? 1 : 0, ClickAction::Throw);
                return true;
            }
        }

        return false;
    }

    /**
     * @brief 处理快捷栏交换（数字键/F键）
     * @param hotbarIndex 快捷栏索引(0-8)或40(副手)
     * @return 是否处理了事件
     */
    bool _handleHotbarSwap(i32 hotbarIndex)
    {
        if (m_menu == nullptr || m_hoveredSlotIndex < 0) {
            return false;
        }

        mc::Slot* slot = m_menu->getSlot(m_hoveredSlotIndex);
        if (slot == nullptr) {
            return false;
        }

        _sendSlotClick(*slot, m_hoveredSlotIndex, hotbarIndex, ClickAction::Swap);
        return true;
    }

    /**
     * @brief 完成拖拽分发
     *
     * 将拖拽操作发送为 QuickCraft 点击序列：
     * 1. START: 发送到 -999 槽位
     * 2. ADD_SLOT: 发送到每个选中的槽位
     * 3. END: 发送到 -999 槽位
     */
    void _finishQuickCraft(bool shiftHeld)
    {
        m_isQuickCrafting = false;

        if (m_quickCraftSlots.empty()) {
            // 没有选中的槽位，取消拖拽
            m_quickCraftSlots.clear();
            return;
        }

        // 如果只有一个槽位，退化为普通点击
        if (m_quickCraftSlots.size() == 1) {
            const i32 slotIndex = m_quickCraftSlots[0];
            mc::Slot* slot = m_menu->getSlot(slotIndex);
            if (slot != nullptr) {
                if (shiftHeld) {
                    _sendSlotClick(*slot, slotIndex, 0, ClickAction::QuickMove);
                } else {
                    _sendSlotClick(*slot, slotIndex, m_quickCraftingButton, ClickAction::Pickup);
                }
            }
            m_quickCraftSlots.clear();
            return;
        }

        // 编码按钮值：低2位 = 事件状态，高2位 = 拖拽模式
        const i32 startButton = (DragConstants::EVENT_START) | (m_quickCraftingType << DragConstants::MODE_SHIFT);
        const i32 addButton = (DragConstants::EVENT_ADD_SLOT) | (m_quickCraftingType << DragConstants::MODE_SHIFT);
        const i32 endButton = (DragConstants::EVENT_END) | (m_quickCraftingType << DragConstants::MODE_SHIFT);

        // 1. START: 发送到 -999 槽位
        _sendOutsideClick(startButton, ClickAction::QuickCraft);

        // 2. ADD_SLOT: 发送到每个选中的槽位
        for (i32 slotIndex : m_quickCraftSlots) {
            mc::Slot* slot = m_menu->getSlot(slotIndex);
            if (slot != nullptr) {
                _sendSlotClick(*slot, slotIndex, addButton, ClickAction::QuickCraft);
            }
        }

        // 3. END: 发送到 -999 槽位
        _sendOutsideClick(endButton, ClickAction::QuickCraft);

        m_quickCraftSlots.clear();
    }

    /**
     * @brief 根据鼠标按钮获取拖拽类型
     * @param button 鼠标按钮 (0=左键, 1=右键, 2=中键)
     * @param ctrlHeld 是否按住Ctrl键
     * @return 拖拽类型
     */
    static i32 _getQuickCraftType(i32 button, bool ctrlHeld)
    {
        (void)ctrlHeld;
        switch (button) {
            case 0:
                return DragConstants::MODE_EVEN; // 左键：均匀分发
            case 1:
                return DragConstants::MODE_SINGLE; // 右键：逐个分发
            case 2:
                return DragConstants::MODE_FILL; // 中键：填满（创造模式）
            default:
                return DragConstants::MODE_EVEN;
        }
    }

    /**
     * @brief 将 ClickAction 和 button 转换为 ClickType
     * @param action 网络点击操作类型
     * @param button 鼠标按钮
     * @return 内部点击类型
     */
    static ClickType _actionToClickType(ClickAction action, i32 button)
    {
        switch (action) {
            case ClickAction::Pickup:
                return (button == 0) ? ClickType::Pick : ClickType::PickSome;
            case ClickAction::QuickMove:
                return ClickType::QuickMove;
            case ClickAction::Swap:
                return ClickType::Swap;
            case ClickAction::Clone:
                return ClickType::Clone;
            case ClickAction::Throw:
                return (button == 0) ? ClickType::Throw : ClickType::ThrowAll;
            case ClickAction::QuickCraft:
                return ClickType::QuickCraft;
            case ClickAction::PickupAll:
                return ClickType::PickAll;
            default:
                return ClickType::Pick;
        }
    }

    /// 点击屏幕外部时的槽位索引
    static constexpr i32 SLOT_CLICKED_OUTSIDE = -999;

    // 拖拽分发状态
    bool m_isQuickCrafting = false;                          ///< 是否正在拖拽分发
    i32 m_quickCraftingButton = -1;                          ///< 拖拽使用的鼠标按钮
    i32 m_quickCraftingType = DragConstants::DRAG_MODE_NONE; ///< 拖拽模式
    std::vector<i32> m_quickCraftSlots;                      ///< 拖拽目标槽位列表
    bool m_skipNextRelease = false;                          ///< 跳过下一次鼠标释放事件

    // 双击检测状态
    mc::Slot* m_lastClickSlot = nullptr;                     ///< 上次点击的槽位
    std::chrono::steady_clock::time_point m_lastClickTime{}; ///< 上次点击时间

    // 当前悬停槽位（用于键盘操作如Q键丢弃、数字键交换）
    i32 m_hoveredSlotIndex = -1; ///< 当前鼠标悬停的槽位索引，在每帧渲染时更新
};

// 静态成员定义
template <typename Menu>
mc::ItemStack AbstractContainerScreen<Menu>::s_emptyStack;

} // namespace mc::client
