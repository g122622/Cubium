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
#include "client/ui/kagero/widget/ContainerWidget.hpp"
#include "client/ui/minecraft/screens/Screen.hpp"
#include "common/screen/IScreen.hpp"
#include <functional>
#include <memory>
#include <variant>
#include <vector>

namespace mc::client::renderer::trident::gui {
class GuiRenderer;
}

namespace mc::client::ui::minecraft::widgets {

/**
 * @brief 屏幕项类型
 *
 * 使用 std::variant 同时支持新的 Screen Widget 和旧的 IScreen
 */
using ScreenItem = std::variant<std::unique_ptr<Screen>, // 新的 Widget-based Screen
    std::unique_ptr<IScreen>                             // 旧的 IScreen 接口
    >;

/**
 * @brief 屏幕栈变化信息
 *
 * 描述屏幕栈发生变化的详细信息，用于回调通知。
 * 当栈为空时，newScreen 和 newIScreen 均为 nullptr。
 * 同一时刻只有一种类型的屏幕位于栈顶。
 */
struct ScreenChangeInfo {
    Screen* newScreen = nullptr;   ///< 变化后的栈顶 Screen（Widget 类型），若非 Widget 类型或栈空则为 nullptr
    IScreen* newIScreen = nullptr; ///< 变化后的栈顶 IScreen（旧接口类型），若非 IScreen 类型或栈空则为 nullptr
    bool stackCleared = false;     ///< 是否因 clear() 导致的栈清空
};

/**
 * @brief 屏幕栈Widget
 *
 * 管理屏幕栈，处理屏幕切换和事件分发。
 * 同时支持新的 Screen Widget 和旧的 IScreen 接口。
 *
 * 屏幕栈：
 * - 支持屏幕堆叠（如暂停菜单覆盖在游戏界面上）
 * - 顶部屏幕接收事件
 * - 底部屏幕可以渲染（如果上层透明）
 *
 * 回调通知：
 * - 在 push/pushIScreen/pop/clear 操作后，通过 ScreenChangeCallback 通知屏幕变化
 * - 同时通过 EventBus 发布 ScreenOpenEvent/ScreenCloseEvent/ScreenChangeEvent 事件
 */
class ScreenStackWidget : public kagero::widget::ContainerWidget {
public:
    /**
     * @brief 屏幕变化回调类型
     *
     * 参数为 ScreenChangeInfo，包含变化后的栈顶屏幕信息。
     */
    using ScreenChangeCallback = std::function<void(const ScreenChangeInfo&)>;

    ScreenStackWidget();
    ~ScreenStackWidget() override = default;

    // ========== 屏幕管理 ==========

    /**
     * @brief 打开新的 Screen Widget
     * @param screen Screen实例
     */
    void push(std::unique_ptr<Screen> screen);

    /**
     * @brief 打开旧的 IScreen
     * @param screen IScreen实例
     * @note 旧的 IScreen 通过适配器集成
     */
    void pushIScreen(std::unique_ptr<IScreen> screen);

    /**
     * @brief 关闭当前屏幕
     */
    void pop();

    /**
     * @brief 关闭所有屏幕
     */
    void clear();

    /**
     * @brief 获取当前 Screen（如果是新Widget类型）
     * @return 栈顶 Screen，如果栈空或类型不对返回nullptr
     */
    [[nodiscard]] Screen* top();
    [[nodiscard]] const Screen* top() const;

    /**
     * @brief 获取当前 IScreen（如果是旧接口类型）
     * @return 栈顶 IScreen，如果栈空或类型不对返回nullptr
     */
    [[nodiscard]] IScreen* topIScreen();
    [[nodiscard]] const IScreen* topIScreen() const;

    /**
     * @brief 检查是否有打开的屏幕
     */
    [[nodiscard]] bool hasScreen() const { return !m_screens.empty(); }

    /**
     * @brief 获取屏幕栈深度
     */
    [[nodiscard]] Size screenCount() const { return m_screens.size(); }

    /**
     * @brief 设置屏幕变化回调
     *
     * 回调在 push/pushIScreen/pop/clear 操作后触发，
     * 参数为 ScreenChangeInfo，包含变化后的栈顶屏幕信息。
     */
    void setScreenChangeCallback(ScreenChangeCallback callback) { m_onScreenChange = std::move(callback); }

    /**
     * @brief 设置部分 tick 时间（用于 IScreen::render）
     */
    void setPartialTick(f32 partialTick) { m_partialTick = partialTick; }

    /**
     * @brief 设置鼠标位置（用于 IScreen::render）
     */
    void setMousePosition(i32 mouseX, i32 mouseY)
    {
        m_mouseX = mouseX;
        m_mouseY = mouseY;
    }

    /**
     * @brief 设置 GuiRenderer（用于 IScreen::render）
     */
    void setGuiRenderer(renderer::trident::gui::GuiRenderer* renderer) { m_guiRenderer = renderer; }

    // ========== Widget接口 ==========

    /**
     * @brief 绘制所有屏幕
     */
    void paint(kagero::widget::PaintContext& ctx) override;

    /**
     * @brief 每帧更新
     */
    void tick(f32 dt) override;

    /**
     * @brief 处理鼠标点击
     */
    bool onClick(i32 mouseX, i32 mouseY, i32 button) override;

    /**
     * @brief 处理鼠标释放
     */
    bool onRelease(i32 mouseX, i32 mouseY, i32 button) override;

    /**
     * @brief 处理鼠标拖动
     */
    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY) override;

    /**
     * @brief 处理鼠标滚轮
     */
    bool onScroll(i32 mouseX, i32 mouseY, f64 delta) override;

    /**
     * @brief 处理键盘按键
     */
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

    /**
     * @brief 处理字符输入
     */
    bool onChar(u32 codePoint) override;

    /**
     * @brief 窗口尺寸改变时调用
     */
    void onResize(i32 width, i32 height) override;

    /**
     * @brief 检查游戏是否应该暂停
     */
    [[nodiscard]] bool shouldPauseGame() const;

private:
    /**
     * @brief 内部屏幕包装器
     *
     * 封装 ScreenItem 并提供统一的接口
     */
    struct ScreenWrapper {
        ScreenItem item;
        bool modal = true; // 在 _onOpenScreen 中从 Screen::isModal() 初始化

        // 公共状态
        bool visible = true;
        bool active = true;

        /**
         * @brief 检查是否为 Widget Screen
         * @note 从 variant 索引推导，无需存储
         */
        [[nodiscard]] bool isWidgetScreen() const { return item.index() == 0; }
    };

    std::vector<ScreenWrapper> m_screens;
    ScreenChangeCallback m_onScreenChange;

    // 拖动状态
    bool m_isDragging = false;
    i32 m_dragButton = 0;
    i32 m_lastMouseX = 0;
    i32 m_lastMouseY = 0;

    // IScreen::render 需要的参数
    f32 m_partialTick = 0.0f;
    i32 m_mouseX = 0;
    i32 m_mouseY = 0;
    renderer::trident::gui::GuiRenderer* m_guiRenderer = nullptr;

    // 辅助方法
    void _onOpenScreen(ScreenWrapper& wrapper);
    void _onCloseScreen(ScreenWrapper& wrapper);
    [[nodiscard]] bool _isScreenModal(const ScreenWrapper& wrapper) const;

    /**
     * @brief 构建当前栈顶的屏幕变化信息
     * @return ScreenChangeInfo，包含栈顶屏幕指针
     */
    [[nodiscard]] ScreenChangeInfo _buildChangeInfo() const;

    /**
     * @brief 通知屏幕变化
     *
     * 触发回调并发布 EventBus 事件。
     * @param openedScreenId 新打开屏幕的标识（仅 push 时设置）
     * @param closedScreenId 关闭屏幕的标识（仅 pop/clear 时设置）
     */
    void _notifyScreenChange(const std::string& openedScreenId = "", const std::string& closedScreenId = "");
};

} // namespace mc::client::ui::minecraft::widgets
