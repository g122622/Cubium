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

#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::client::ui::kagero::widget {

// 前向声明
class IWidgetContainer;
class PaintContext;

/**
 * @brief UI音效回调类型
 *
 * 用于播放UI音效（如按钮点击）。
 * 参数：音效事件ID（如 "minecraft:ui.button.click"）
 */
using UiSoundCallback = std::function<void(const std::string& soundEventId)>;

/**
 * @brief Widget基类
 *
 * 所有UI组件的基类，提供：
 * - 生命周期管理（init, tick, paint）
 * - 事件处理（click, drag, scroll, key, char）
 * - 布局属性（position, size, anchor）
 * - 状态管理（visible, active, hovered, focused）
 * - UI音效支持（静态回调注入）
 *
 *
 * 使用示例：
 * @code
 * class MyButton : public Widget {
 * public:
 *     void paint(PaintContext& ctx) override {
 *         ctx.drawFilledRect(bounds(), getBackgroundColor());
 *         ctx.drawBorder(bounds(), 1.0f, isHovered() ? hoverColor : borderColor);
 *     }
 *
 *     bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods) override {
 *         if (button == 0) {
 *             // 处理左键点击
 *             return true;
 *         }
 *         return false;
 *     }
 * };
 * @endcode
 */
class Widget {
public:
    using Ptr = std::unique_ptr<Widget>;
    using WeakPtr = Widget*;

    /**
     * @brief 构造函数
     */
    Widget() = default;

    /**
     * @brief 构造函数（带ID）
     * @param id 组件ID
     */
    explicit Widget(std::string id)
        : m_id(std::move(id))
    {}

    /**
     * @brief 虚析构函数
     */
    virtual ~Widget() = default;

    // 禁止拷贝
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    // 允许移动
    Widget(Widget&&) noexcept = default;
    Widget& operator=(Widget&&) noexcept = default;

    // ==================== 生命周期 ====================

    /**
     * @brief 初始化组件
     *
     * 在组件被添加到屏幕或容器时调用一次
     */
    virtual void init() {}

    /**
     * @brief 每帧更新
     * @param dt 增量时间（秒）
     */
    virtual void tick(f32 dt) { (void)dt; }

    /**
     * @brief 绘制组件
     *
     * 子类应重写此方法实现绘制逻辑。
     * 悬停状态由事件系统在渲染前通过 updateHover() 更新，
     * paint() 只负责纯粹的绘制操作。
     *
     * @param ctx 绘图上下文
     */
    virtual void paint(PaintContext& ctx) { (void)ctx; }

    /**
     * @brief 窗口尺寸改变时调用
     * @param width 新宽度
     * @param height 新高度
     */
    virtual void onResize(i32 width, i32 height)
    {
        (void)width;
        (void)height;
    }

    // ==================== 事件处理 ====================

    /**
     * @brief 鼠标点击事件
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param button 鼠标按钮
     * @param mods 修饰键位掩码 (GLFW_MOD_SHIFT, GLFW_MOD_CONTROL 等)
     * @return 如果事件被处理返回true
     */
    virtual bool onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods)
    {
        (void)mouseX;
        (void)mouseY;
        (void)button;
        (void)mods;
        return false;
    }

    /**
     * @brief 鼠标释放事件
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param button 鼠标按钮
     * @param mods 修饰键位掩码 (GLFW_MOD_SHIFT, GLFW_MOD_CONTROL 等)
     * @return 如果事件被处理返回true
     */
    virtual bool onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods)
    {
        (void)mouseX;
        (void)mouseY;
        (void)button;
        (void)mods;
        return false;
    }

    /**
     * @brief 鼠标拖动事件
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param deltaX X方向移动量
     * @param deltaY Y方向移动量
     * @param button 触发拖动的鼠标按钮 (0=左键, 1=右键, 2=中键)
     * @return 如果事件被处理返回true
     */
    virtual bool onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button)
    {
        (void)mouseX;
        (void)mouseY;
        (void)deltaX;
        (void)deltaY;
        (void)button;
        return false;
    }

    /**
     * @brief 鼠标滚轮事件
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @param delta 滚轮增量
     * @return 如果事件被处理返回true
     */
    virtual bool onScroll(i32 mouseX, i32 mouseY, f64 delta)
    {
        (void)mouseX;
        (void)mouseY;
        (void)delta;
        return false;
    }

    /**
     * @brief 键盘按键事件
     * @param key 键码（GLFW键码）
     * @param scanCode 扫描码
     * @param action 动作（按下/释放/重复）
     * @param mods 修饰键
     * @return 如果事件被处理返回true
     */
    virtual bool onKey(i32 key, i32 scanCode, i32 action, i32 mods)
    {
        (void)key;
        (void)scanCode;
        (void)action;
        (void)mods;
        return false;
    }

    /**
     * @brief 字符输入事件
     * @param codePoint Unicode码点
     * @return 如果事件被处理返回true
     */
    virtual bool onChar(u32 codePoint)
    {
        (void)codePoint;
        return false;
    }

    /**
     * @brief 鼠标进入组件
     */
    virtual void onMouseEnter() {}

    /**
     * @brief 鼠标离开组件
     */
    virtual void onMouseLeave() {}

    /**
     * @brief 鼠标移动事件
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     * @return 如果事件被处理返回true
     */
    virtual bool onMouseMove(i32 mouseX, i32 mouseY)
    {
        (void)mouseX;
        (void)mouseY;
        return false;
    }

    /**
     * @brief 获得焦点
     */
    virtual void onFocusGained() {}

    /**
     * @brief 失去焦点
     */
    virtual void onFocusLost() {}

    // ==================== 布局 ====================

    /**
     * @brief 设置位置
     * @param x X坐标
     * @param y Y坐标
     */
    void setPosition(i32 x, i32 y)
    {
        m_bounds.x = x;
        m_bounds.y = y;
        onPositionChanged();
    }

    /**
     * @brief 设置尺寸
     * @param width 宽度
     * @param height 高度
     */
    void setSize(i32 width, i32 height)
    {
        m_bounds.width = width;
        m_bounds.height = height;
        onSizeChanged();
    }

    /**
     * @brief 设置边界
     * @param rect 矩形边界
     */
    void setBounds(const Rect& rect)
    {
        m_bounds = rect;
        onPositionChanged();
        onSizeChanged();
    }

    /**
     * @brief 设置锚点
     * @param anchor 锚点位置
     */
    void setAnchor(Anchor anchor)
    {
        m_anchor = anchor;
        onPositionChanged();
    }

    /**
     * @brief 设置边距
     * @param margin 边距
     */
    void setMargin(const Margin& margin) { m_margin = margin; }

    /**
     * @brief 设置内边距
     * @param padding 内边距
     */
    void setPadding(const Padding& padding) { m_padding = padding; }

    // ==================== 状态查询 ====================

    /**
     * @brief 检查是否可见
     */
    [[nodiscard]] bool isVisible() const { return m_visible; }

    /**
     * @brief 检查是否激活（可交互）
     */
    [[nodiscard]] bool isActive() const { return m_active; }

    /**
     * @brief 检查鼠标是否悬停
     */
    [[nodiscard]] bool isHovered() const { return m_hovered; }

    /**
     * @brief 检查是否获得焦点
     */
    [[nodiscard]] bool isFocused() const { return m_focused; }

    /**
     * @brief 检查组件是否被禁用
     */
    [[nodiscard]] bool isDisabled() const { return !m_active; }

    /**
     * @brief 检查点是否在组件内
     * @param x X坐标
     * @param y Y坐标
     */
    [[nodiscard]] bool contains(i32 x, i32 y) const { return m_visible && m_bounds.contains(x, y); }

    /**
     * @brief 检查鼠标是否在组件上
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     */
    [[nodiscard]] bool isMouseOver(i32 mouseX, i32 mouseY) const
    {
        return m_active && m_visible && contains(mouseX, mouseY);
    }

    // ==================== 状态设置 ====================

    /**
     * @brief 设置可见性
     */
    void setVisible(bool visible)
    {
        if (m_visible != visible) {
            m_visible = visible;
            onVisibilityChanged(visible);
        }
    }

    /**
     * @brief 设置激活状态
     */
    void setActive(bool active)
    {
        if (m_active != active) {
            m_active = active;
            onActiveChanged(active);
        }
    }

    /**
     * @brief 设置焦点
     */
    void setFocused(bool focused)
    {
        if (m_focused != focused) {
            m_focused = focused;
            if (focused) {
                onFocusGained();
            } else {
                onFocusLost();
            }
        }
    }

    /**
     * @brief 设置悬停状态（由容器调用）
     */
    void setHovered(bool hovered)
    {
        if (m_hovered != hovered) {
            m_hovered = hovered;
            if (hovered) {
                onMouseEnter();
            } else {
                onMouseLeave();
            }
        }
    }

    // ==================== 层级 ====================

    /**
     * @brief 设置Z索引
     */
    void setZIndex(i32 z) { m_zIndex = z; }

    /**
     * @brief 获取Z索引
     */
    [[nodiscard]] i32 zIndex() const { return m_zIndex; }

    /**
     * @brief 设置父容器
     */
    void setParent(IWidgetContainer* parent) { m_parent = parent; }

    /**
     * @brief 获取父容器
     */
    [[nodiscard]] IWidgetContainer* parent() const { return m_parent; }

    // ==================== 属性访问 ====================

    /**
     * @brief 获取组件ID
     */
    [[nodiscard]] const std::string& id() const { return m_id; }

    /**
     * @brief 设置组件ID
     */
    void setId(std::string id) { m_id = std::move(id); }

    // ==================== 自定义数据 ====================

    /**
     * @brief 设置自定义字符串数据
     * @param key 数据键
     * @param value 数据值
     */
    void setUserData(const std::string& key, const std::string& value) { m_userData[key] = value; }

    /**
     * @brief 获取自定义字符串数据
     * @param key 数据键
     * @return 数据值指针，不存在则返回 nullptr
     */
    [[nodiscard]] const std::string* getUserData(const std::string& key) const
    {
        auto it = m_userData.find(key);
        return it != m_userData.end() ? &it->second : nullptr;
    }

    /**
     * @brief 获取边界
     */
    [[nodiscard]] const Rect& bounds() const { return m_bounds; }

    /**
     * @brief 获取X坐标
     */
    [[nodiscard]] i32 x() const { return m_bounds.x; }

    /**
     * @brief 获取Y坐标
     */
    [[nodiscard]] i32 y() const { return m_bounds.y; }

    /**
     * @brief 获取宽度
     */
    [[nodiscard]] i32 width() const { return m_bounds.width; }

    /**
     * @brief 获取高度
     */
    [[nodiscard]] i32 height() const { return m_bounds.height; }

    /**
     * @brief 获取锚点
     */
    [[nodiscard]] Anchor anchor() const { return m_anchor; }

    /**
     * @brief 获取边距
     */
    [[nodiscard]] const Margin& margin() const { return m_margin; }

    /**
     * @brief 获取内边距
     */
    [[nodiscard]] const Padding& padding() const { return m_padding; }

    /**
     * @brief 获取透明度
     */
    [[nodiscard]] f32 alpha() const { return m_alpha; }

    /**
     * @brief 设置透明度
     */
    void setAlpha(f32 alpha) { m_alpha = alpha; }

    /**
     * @brief 获取背景色
     */
    [[nodiscard]] u32 backgroundColor() const { return m_backgroundColor; }

    /**
     * @brief 设置背景色
     */
    void setBackgroundColor(u32 color) { m_backgroundColor = color; }

    /**
     * @brief 获取边框色
     */
    [[nodiscard]] u32 borderColor() const { return m_borderColor; }

    /**
     * @brief 设置边框色
     */
    void setBorderColor(u32 color) { m_borderColor = color; }

    /**
     * @brief 获取圆角半径
     */
    [[nodiscard]] i32 cornerRadius() const { return m_cornerRadius; }

    /**
     * @brief 设置圆角半径
     */
    void setCornerRadius(i32 radius) { m_cornerRadius = radius; }

    /**
     * @brief 更新悬停状态
     *
     * 由容器调用以更新悬停状态
     *
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     */
    virtual void updateHover(i32 mouseX, i32 mouseY) { setHovered(isMouseOver(mouseX, mouseY)); }

    // ==================== UI音效支持 ====================

    /**
     * @brief 设置UI音效回调
     *
     * 此回调用于播放UI音效（如按钮点击音）。
     * 应在应用程序初始化时调用，将音频服务连接到UI组件。
     *
     * @param callback 音效回调函数，参数为音效事件ID（如 "minecraft:ui.button.click"）
     *
     * @code
     * // 在 ClientApplication 初始化时设置
     * Widget::setUiSoundCallback([this](const std::string& soundEventId) {
     *     if (m_audioService) {
     *         auto sound = std::make_unique<sound::SoundInstance>(
     *             sound::SoundInstance::createGlobal(
     *                 ResourceLocation(soundEventId),
     *                 sound::SoundCategory::Master,
     *                 0.25f,
     *                 1.0f
     *             )
     *         );
     *         m_audioService->play(std::move(sound));
     *     }
     * });
     * @endcode
     */
    static void setUiSoundCallback(UiSoundCallback callback) { s_uiSoundCallback = std::move(callback); }

    /**
     * @brief 获取UI音效回调
     */
    [[nodiscard]] static const UiSoundCallback& uiSoundCallback() { return s_uiSoundCallback; }

    /**
     * @brief 播放UI音效
     *
     * 如果设置了音效回调，则调用回调播放音效。
     * 子类可在事件处理中调用此方法播放UI音效。
     *
     * @param soundEventId 音效事件ID（如 "minecraft:ui.button.click"）
     */
    static void playUiSound(const std::string& soundEventId)
    {
        if (s_uiSoundCallback) {
            s_uiSoundCallback(soundEventId);
        }
    }

protected:
    /**
     * @brief 位置改变时调用
     */
    virtual void onPositionChanged() {}

    /**
     * @brief 尺寸改变时调用
     */
    virtual void onSizeChanged() {}

    /**
     * @brief 可见性改变时调用
     */
    virtual void onVisibilityChanged(bool visible) { (void)visible; }

    /**
     * @brief 激活状态改变时调用
     */
    virtual void onActiveChanged(bool active) { (void)active; }

    // 成员变量
    std::string m_id;                                        ///< 组件ID
    Rect m_bounds;                                           ///< 边界矩形
    Anchor m_anchor = Anchor::TopLeft;                       ///< 锚点
    Margin m_margin;                                         ///< 边距
    Padding m_padding;                                       ///< 内边距
    bool m_visible = true;                                   ///< 是否可见
    bool m_active = true;                                    ///< 是否激活（可交互）
    bool m_hovered = false;                                  ///< 鼠标悬停
    bool m_focused = false;                                  ///< 是否获得焦点
    i32 m_zIndex = 0;                                        ///< Z索引
    f32 m_alpha = 1.0f;                                      ///< 透明度
    u32 m_backgroundColor = Colors::fromARGB(0, 0, 0, 0);    ///< 背景色
    u32 m_borderColor = Colors::fromARGB(0, 0, 0, 0);        ///< 边框色
    i32 m_cornerRadius = 0;                                  ///< 圆角半径
    IWidgetContainer* m_parent = nullptr;                    ///< 父容器
    std::unordered_map<std::string, std::string> m_userData; ///< 自定义数据存储

    // 静态成员：UI音效回调（inline 允许在头文件中定义）
    inline static UiSoundCallback s_uiSoundCallback;
};

} // namespace mc::client::ui::kagero::widget
