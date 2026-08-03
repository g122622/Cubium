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

#include "TemplateScreen.hpp"
#include "client/ui/kagero/widget/SlotWidget.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/container/LoomContainer.hpp"
#include "core/Types.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mc {
class PlayerInventory;
class ItemStack;
} // namespace mc

namespace mc::client::ui::minecraft {

/**
 * @brief 织布机屏幕
 *
 * 使用Kagero声明式模板驱动的织布机GUI界面。
 *
 * 布局参考MC 1.16.5织布机：
 * - 左侧：图案选择区域（可滚动的4x4网格）
 * - 中上：旗帜槽 + 染料槽 + 图案物品槽
 * - 右侧：输出槽
 * - 下方：玩家背包 + 快捷栏
 *
 * 图案选择：
 * - 无图案物品时：显示35个基础图案（可滚动）
 * - 有图案物品时：仅显示1个对应图案
 * - 选中的图案高亮显示
 *
 * 参考: net.minecraft.client.gui.screen.inventory.LoomScreen
 */
class LoomScreen : public TemplateScreen {
public:
    using ContainerClickSender =
        std::function<void(mc::ContainerId, i32, i32, i16, mc::ClickAction, const mc::ItemStack&)>;
    using ContainerCloseSender = std::function<void(mc::ContainerId)>;

    /**
     * @brief 构造函数
     * @param containerId 容器ID
     * @param playerInventory 玩家背包
     * @param clickSender 容器点击事件发送器
     * @param closeSender 容器关闭事件发送器
     */
    LoomScreen(mc::ContainerId containerId,
        mc::PlayerInventory* playerInventory,
        ContainerClickSender clickSender = {},
        ContainerCloseSender closeSender = {});

    ~LoomScreen() override = default;

    LoomScreen(const LoomScreen&) = delete;
    LoomScreen& operator=(const LoomScreen&) = delete;
    LoomScreen(LoomScreen&&) noexcept = default;
    LoomScreen& operator=(LoomScreen&&) noexcept = default;

    /**
     * @brief 获取屏幕标题
     */
    [[nodiscard]] std::string getTitle() const { return "Loom"; }

    /**
     * @brief 键盘事件处理（ESC/E关闭）
     */
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

protected:
    void onOpen() override;
    void onClose() override;
    void tick(f32 dt) override;

private:
    /**
     * @brief 注册模板回调和绑定
     */
    void _registerCallbacks();

    /**
     * @brief 初始化槽位组件
     *
     * 将LoomContainer的槽位映射到模板中的SlotWidget。
     */
    void _initSlots();

    /**
     * @brief 初始化图案选择区域
     *
     * 根据输入物品状态创建图案按钮。
     */
    void _initPatternButtons();

    /**
     * @brief 刷新所有槽位显示
     */
    void _refreshSlots();

    /**
     * @brief 刷新图案选择区域
     *
     * 根据当前输入物品和选中的图案更新图案按钮状态。
     */
    void _refreshPatterns();

    /**
     * @brief 处理槽位点击
     * @param slotIndex 槽位索引
     * @param button 鼠标按钮
     * @param shiftHeld 是否按住Shift
     */
    void _onSlotClick(i32 slotIndex, i32 button, bool shiftHeld);

    /**
     * @brief 处理图案选择
     * @param patternIndex 图案索引（1-based）
     */
    void _onPatternSelect(i32 patternIndex);

    /**
     * @brief 处理图案区域滚动
     * @param deltaX 水平滚动量
     * @param deltaY 垂直滚动量
     */
    void _onPatternScroll(i32 x, i32 y, f64 deltaX, f64 deltaY);

    /**
     * @brief 处理关闭屏幕
     */
    void _onCloseScreen();

    // ========== 容器 ==========

    std::unique_ptr<mc::entity::inventory::container::LoomContainer> m_menu;
    mc::PlayerInventory* m_playerInventory = nullptr;
    ContainerClickSender m_clickSender;
    ContainerCloseSender m_closeSender;

    // ========== 图案选择状态 ==========

    /// 当前显示的起始图案索引
    i32 m_patternStartIndex = 0;
    /// 选中图案的按钮组件ID列表
    std::vector<std::string> m_patternButtonIds;

    // ========== 布局常量 ==========

    static constexpr i32 GUI_WIDTH = 176;
    static constexpr i32 GUI_HEIGHT = 166;
    static constexpr i32 PATTERN_GRID_COLS = 4;
    static constexpr i32 PATTERN_GRID_ROWS = 4;
    static constexpr i32 PATTERN_BUTTON_SIZE = 14;
    static constexpr i32 PATTERN_BUTTON_SPACING = 18;
};

} // namespace mc::client::ui::minecraft
