/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to furnished do so, subject to the following conditions:
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

#include "ScreenStackWidget.hpp"
#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/event/UIEvents.hpp"

namespace mc::client::ui::minecraft::widgets {

ScreenStackWidget::ScreenStackWidget()
    : ContainerWidget()
{
    setId("screen_stack");
    setVisible(true);
    setActive(true);
}

void ScreenStackWidget::push(std::unique_ptr<Screen> screen)
{
    if (screen == nullptr) {
        return;
    }

    // 记录打开的屏幕标识和之前的栈顶，用于 EventBus 事件
    std::string openedId = screen->id();
    std::string fromId = _getTopScreenId();

    ScreenWrapper wrapper;
    wrapper.item = std::move(screen);
    wrapper.visible = true;
    wrapper.active = true;
    wrapper.modal = true; // 默认模态

    _onOpenScreen(wrapper);
    m_screens.push_back(std::move(wrapper));

    _notifyScreenChange(fromId, openedId, "");
}

void ScreenStackWidget::pop()
{
    if (m_screens.empty()) {
        return;
    }

    // 记录关闭的屏幕标识，用于 EventBus 事件
    std::string closedId = _getScreenId(m_screens.back());

    _onCloseScreen(m_screens.back());
    m_screens.pop_back();

    // 关闭后的新栈顶即为 fromId 方向
    std::string fromId = closedId;
    _notifyScreenChange(fromId, "", closedId);
}

void ScreenStackWidget::clear()
{
    if (m_screens.empty()) {
        return;
    }

    auto& bus = kagero::event::EventBus::instance();

    // 为每个被关闭的屏幕发布 ScreenCloseEvent，与 pop() 行为一致
    while (!m_screens.empty()) {
        std::string closedId = _getScreenId(m_screens.back());
        _onCloseScreen(m_screens.back());
        m_screens.pop_back();

        if (!closedId.empty()) {
            bus.publish(kagero::event::ScreenCloseEvent(closedId));
        }
    }

    ScreenChangeInfo info;
    info.newScreen = nullptr;
    info.stackCleared = true;

    if (m_onScreenChange) {
        m_onScreenChange(info);
    }

    // 发布屏幕切换事件（从某个屏幕变为空）
    bus.publish(kagero::event::ScreenChangeEvent("", ""));
}

Screen* ScreenStackWidget::top()
{
    if (m_screens.empty()) {
        return nullptr;
    }
    return m_screens.back().item.get();
}

const Screen* ScreenStackWidget::top() const
{
    if (m_screens.empty()) {
        return nullptr;
    }
    return m_screens.back().item.get();
}

void ScreenStackWidget::_onOpenScreen(ScreenWrapper& wrapper)
{
    auto* screen = wrapper.item.get();
    if (screen) {
        screen->onOpen();
        wrapper.modal = screen->isModal();
    }
}

void ScreenStackWidget::_onCloseScreen(ScreenWrapper& wrapper)
{
    auto* screen = wrapper.item.get();
    if (screen) {
        screen->onClose();
    }
}

bool ScreenStackWidget::_isScreenModal(const ScreenWrapper& wrapper) const
{
    return wrapper.modal;
}

ScreenChangeInfo ScreenStackWidget::_buildChangeInfo() const
{
    ScreenChangeInfo info;
    if (m_screens.empty()) {
        info.newScreen = nullptr;
    } else {
        info.newScreen = m_screens.back().item.get();
    }
    info.stackCleared = false;
    return info;
}

std::string ScreenStackWidget::_getScreenId(const ScreenWrapper& wrapper) const
{
    auto* s = wrapper.item.get();
    return s ? s->id() : "";
}

std::string ScreenStackWidget::_getTopScreenId() const
{
    if (m_screens.empty()) {
        return "";
    }
    return _getScreenId(m_screens.back());
}

void ScreenStackWidget::_notifyScreenChange(
    const std::string& fromId, const std::string& openedScreenId, const std::string& closedScreenId)
{
    // 触发回调
    if (m_onScreenChange) {
        m_onScreenChange(_buildChangeInfo());
    }

    // 发布 EventBus 事件
    auto& bus = kagero::event::EventBus::instance();

    if (!openedScreenId.empty()) {
        bus.publish(kagero::event::ScreenOpenEvent(openedScreenId));
    }

    if (!closedScreenId.empty()) {
        bus.publish(kagero::event::ScreenCloseEvent(closedScreenId));
    }

    // 总是发布切换事件
    std::string toId = _getTopScreenId();
    bus.publish(kagero::event::ScreenChangeEvent(fromId, toId));
}

void ScreenStackWidget::paint(kagero::widget::PaintContext& ctx)
{
    if (m_screens.empty()) {
        return;
    }

    // 从顶层向下找到第一个模态屏幕，模态屏幕下方的层不需要渲染
    size_t firstPaintIndex = 0;
    for (auto it = m_screens.rbegin(); it != m_screens.rend(); ++it) {
        if (it->visible) {
            firstPaintIndex = std::distance(it, m_screens.rend()) - 1;
        }
        if (_isScreenModal(*it)) {
            break;
        }
    }

    // 从最底层的可见层渲染到顶层
    for (size_t i = firstPaintIndex; i < m_screens.size(); ++i) {
        const auto& wrapper = m_screens[i];
        if (!wrapper.visible) {
            continue;
        }

        auto* screen = wrapper.item.get();
        if (screen) {
            screen->paint(ctx);
        }
    }
}

void ScreenStackWidget::tick(f32 dt)
{
    // 更新所有屏幕
    for (const auto& wrapper : m_screens) {
        if (!wrapper.visible || !wrapper.active) {
            continue;
        }

        auto* screen = wrapper.item.get();
        if (screen) {
            screen->tick(dt);
        }
    }
}

bool ScreenStackWidget::onClick(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    (void)mods;
    // 从顶层开始处理
    for (auto it = m_screens.rbegin(); it != m_screens.rend(); ++it) {
        const auto& wrapper = *it;
        if (!wrapper.visible || !wrapper.active) {
            continue;
        }

        bool handled = false;
        auto* screen = wrapper.item.get();
        if (screen) {
            handled = screen->onClick(mouseX, mouseY, button, mods);
        }

        if (handled) {
            m_isDragging = true;
            m_dragButton = button;
            m_lastMouseX = mouseX;
            m_lastMouseY = mouseY;
            return true;
        }

        // 如果屏幕是模态的，阻止事件向下传播
        if (_isScreenModal(wrapper)) {
            return false;
        }
    }
    return false;
}

bool ScreenStackWidget::onRelease(i32 mouseX, i32 mouseY, i32 button, i32 mods)
{
    (void)mods;
    if (m_isDragging && m_dragButton == button) {
        m_isDragging = false;
        m_dragButton = 0;

        // 发送释放事件到顶层屏幕
        if (!m_screens.empty()) {
            const auto& wrapper = m_screens.back();
            if (wrapper.visible && wrapper.active) {
                auto* screen = wrapper.item.get();
                if (screen) {
                    return screen->onRelease(mouseX, mouseY, button, mods);
                }
            }
        }
    }
    return false;
}

bool ScreenStackWidget::onDrag(i32 mouseX, i32 mouseY, i32 deltaX, i32 deltaY, i32 button)
{
    (void)button;
    if (m_isDragging && !m_screens.empty()) {
        const auto& wrapper = m_screens.back();
        if (wrapper.visible && wrapper.active) {
            auto* screen = wrapper.item.get();
            if (screen) {
                return screen->onDrag(mouseX, mouseY, deltaX, deltaY, button);
            }
        }
    }
    return false;
}

bool ScreenStackWidget::onScroll(i32 mouseX, i32 mouseY, f64 delta)
{
    // 从顶层开始处理
    for (auto it = m_screens.rbegin(); it != m_screens.rend(); ++it) {
        const auto& wrapper = *it;
        if (!wrapper.visible || !wrapper.active) {
            continue;
        }

        bool handled = false;
        auto* screen = wrapper.item.get();
        if (screen) {
            handled = screen->onScroll(mouseX, mouseY, delta);
        }

        if (handled) {
            return true;
        }
        if (_isScreenModal(wrapper)) {
            return false;
        }
    }
    return false;
}

bool ScreenStackWidget::onKey(i32 key, i32 scanCode, i32 action, i32 mods)
{
    // 从顶层开始处理
    for (auto it = m_screens.rbegin(); it != m_screens.rend(); ++it) {
        const auto& wrapper = *it;
        if (!wrapper.visible || !wrapper.active) {
            continue;
        }

        bool handled = false;
        auto* screen = wrapper.item.get();
        if (screen) {
            handled = screen->onKey(key, scanCode, action, mods);
        }

        if (handled) {
            return true;
        }
        if (_isScreenModal(wrapper)) {
            return false;
        }
    }
    return false;
}

bool ScreenStackWidget::onChar(u32 codePoint)
{
    // 从顶层开始处理
    for (auto it = m_screens.rbegin(); it != m_screens.rend(); ++it) {
        const auto& wrapper = *it;
        if (!wrapper.visible || !wrapper.active) {
            continue;
        }

        bool handled = false;
        auto* screen = wrapper.item.get();
        if (screen) {
            handled = screen->onChar(codePoint);
        }

        if (handled) {
            return true;
        }
        if (_isScreenModal(wrapper)) {
            return false;
        }
    }
    return false;
}

void ScreenStackWidget::onResize(i32 width, i32 height)
{
    // 通知所有屏幕尺寸变化
    for (const auto& wrapper : m_screens) {
        auto* screen = wrapper.item.get();
        if (screen) {
            screen->setBounds(kagero::Rect(0, 0, width, height));
            screen->onResize(width, height);
        }
    }
}

bool ScreenStackWidget::shouldPauseGame() const
{
    // 检查是否有暂停屏幕
    for (const auto& wrapper : m_screens) {
        auto* screen = wrapper.item.get();
        if (screen && screen->isPauseScreen()) {
            return true;
        }
    }
    return false;
}

} // namespace mc::client::ui::minecraft::widgets
