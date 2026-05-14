#pragma once

#include "../../kagero/paint/PaintContext.hpp"
#include "../../kagero/widget/ContainerWidget.hpp"
#include "../screens/Screen.hpp"
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
 * @brief 屏幕栈Widget
 *
 * 管理屏幕栈，处理屏幕切换和事件分发。
 * 同时支持新的 Screen Widget 和旧的 IScreen 接口。
 *
 * 屏幕栈：
 * - 支持屏幕堆叠（如暂停菜单覆盖在游戏界面上）
 * - 顶部屏幕接收事件
 * - 底部屏幕可以渲染（如果上层透明）
 */
class ScreenStackWidget : public kagero::widget::ContainerWidget {
public:
    /**
     * @brief 屏幕变化回调类型
     */
    using ScreenChangeCallback = std::function<void(Screen*)>;

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
    [[nodiscard]] size_t screenCount() const { return m_screens.size(); }

    /**
     * @brief 设置屏幕变化回调
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
        bool modal = true; // 在 onOpenScreen 中从 Screen::isModal() 初始化

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
    void onOpenScreen(ScreenWrapper& wrapper);
    void onCloseScreen(ScreenWrapper& wrapper);
    [[nodiscard]] bool isScreenModal(const ScreenWrapper& wrapper) const;
};

} // namespace mc::client::ui::minecraft::widgets
