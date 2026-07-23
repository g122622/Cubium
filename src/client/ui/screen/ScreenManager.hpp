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

#include "client/ui/minecraft/widgets/ScreenStackWidget.hpp"
#include <memory>

namespace mc::client {

/**
 * @brief 屏幕管理器
 *
 * 管理屏幕栈，处理屏幕切换和事件分发。
 * 委托给 ScreenStackWidget 进行实际管理。
 *
 * 屏幕栈：
 * - 支持屏幕堆叠（如聊天界面覆盖在游戏界面上）
 * - 顶部屏幕接收事件
 * - 底部屏幕可以渲染（如果上层透明）
 *
 * 使用示例：
 * @code
 * ScreenManager& manager = ScreenManager::instance();
 * manager.openScreen(std::make_unique<InventoryScreen>(...));
 * manager.closeScreen();
 * @endcode
 */
class ScreenManager {
public:
    /**
     * @brief 获取单例实例
     * @return 屏幕管理器实例引用
     */
    static ScreenManager& instance() noexcept;

    /**
     * @brief 设置 ScreenStackWidget 后端
     * @param stackWidget ScreenStackWidget 实例
     */
    void setScreenStackWidget(ui::minecraft::widgets::ScreenStackWidget* stackWidget);

    /**
     * @brief 打开 kagero Screen（Widget 体系）
     *
     * 将屏幕推入栈顶并触发 onOpen。
     * @param screen kagero Screen 实例
     */
    void openScreen(std::unique_ptr<ui::minecraft::Screen> screen);

    /**
     * @brief 关闭当前屏幕
     *
     * 关闭栈顶屏幕并恢复下层屏幕。
     */
    void closeScreen();

    /**
     * @brief 关闭所有屏幕
     */
    void closeAll();

    /**
     * @brief 获取当前 kagero Screen（Widget 体系）
     * @return 栈顶 kagero Screen，若栈空返回 nullptr
     */
    [[nodiscard]] ui::minecraft::Screen* getCurrentKageroScreen() noexcept
    {
        return m_stackWidget ? m_stackWidget->top() : nullptr;
    }
    [[nodiscard]] const ui::minecraft::Screen* getCurrentKageroScreen() const noexcept
    {
        return m_stackWidget ? m_stackWidget->top() : nullptr;
    }

    /**
     * @brief 检查是否有打开的屏幕
     * @return 如果有屏幕返回true
     */
    [[nodiscard]] bool hasScreen() const noexcept { return m_stackWidget ? m_stackWidget->hasScreen() : false; }

    /**
     * @brief 获取屏幕栈深度
     * @return 屏幕数量
     */
    [[nodiscard]] size_t getScreenCount() const noexcept { return m_stackWidget ? m_stackWidget->screenCount() : 0; }

    /**
     * @brief 每帧更新
     * @param dt 增量时间
     */
    void tick(f32 dt);

    /**
     * @brief 渲染所有屏幕
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param partialTick 部分tick时间
     */
    void render(i32 mouseX, i32 mouseY, f32 partialTick);

    /**
     * @brief 处理鼠标点击
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param button 鼠标按钮
     * @param mods 修饰键位掩码 (GLFW_MOD_SHIFT, GLFW_MOD_CONTROL 等)
     * @return 如果事件被处理返回true
     */
    bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods);

    /**
     * @brief 处理鼠标释放
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param button 鼠标按钮
     * @param mods 修饰键位掩码 (GLFW_MOD_SHIFT, GLFW_MOD_CONTROL 等)
     * @return 如果事件被处理返回true
     */
    bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods);

    /**
     * @brief 处理鼠标拖动
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param deltaX X方向移动量
     * @param deltaY Y方向移动量
     * @param button 触发拖动的鼠标按钮
     * @return 如果事件被处理返回true
     */
    bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button);

    /**
     * @brief 处理鼠标滚轮
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param delta 滚轮增量
     * @return 如果事件被处理返回true
     */
    bool onScroll(i32 mouseX, i32 mouseY, f64 delta);

    /**
     * @brief 处理键盘按键
     * @param key 键码
     * @param scanCode 扫描码
     * @param action 动作
     * @param mods 修饰键
     * @return 如果事件被处理返回true
     */
    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods);

    /**
     * @brief 处理字符输入
     * @param codePoint Unicode码点
     * @return 如果事件被处理返回true
     */
    bool onChar(u32 codePoint);

    /**
     * @brief 窗口尺寸改变时调用
     * @param width 新宽度
     * @param height 新高度
     */
    void onResize(i32 width, i32 height);

    /**
     * @brief 检查游戏是否应该暂停
     * @return 如果有暂停屏幕返回true
     */
    [[nodiscard]] bool shouldPauseGame() const noexcept;

private:
    ScreenManager() = default;
    ~ScreenManager() = default;
    ScreenManager(const ScreenManager&) = delete;
    ScreenManager& operator=(const ScreenManager&) = delete;

    ui::minecraft::widgets::ScreenStackWidget* m_stackWidget = nullptr;
};

} // namespace mc::client
