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

#include "common/core/Types.hpp"
#include "server/application/console/buffer/EventRingBuffer.hpp"
#include "server/application/console/buffer/LogRingBuffer.hpp"

#include <deque>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <ftxui/component/component_base.hpp>
#include <ftxui/screen/box.hpp>

namespace mc::server::console {

/**
 * @brief 左侧日志滚动区
 *
 * 展示日志环形缓冲与事件环形缓冲的最新内容：
 * - 默认"跟随最新"：每帧取缓冲尾部，回到底部时自动恢复；
 * - PgUp/PgDn 与鼠标滚轮回看，回看时暂停跟随；
 * - 提供条数统计，供布局在标题栏显示。
 *
 * 组件本身不是焦点组件（输入框始终持有焦点），键位由外层 CatchEvent 转发。
 */
class LogViewComponent final : public ftxui::ComponentBase {
public:
    /**
     * @brief 构造日志区
     * @param logs 日志缓冲（须比本对象存活更久）
     * @param events 事件缓冲（须比本对象存活更久）
     */
    LogViewComponent(const LogRingBuffer& logs, const EventRingBuffer& events);

    ftxui::Element OnRender() override;

    /**
     * @brief 处理回看相关按键/滚轮（由 ConsoleLayout 统一分发）
     * @param event FTXUI 事件
     * @return 是否已消费
     */
    bool handleScrollEvent(const ftxui::Event& event);

    /**
     * @brief 是否处于"跟随最新"状态
     */
    [[nodiscard]] bool following() const noexcept
    {
        return m_following;
    }

    /**
     * @brief 当前向后回看的行数（0 表示贴底）
     */
    [[nodiscard]] usize scrollOffset() const noexcept
    {
        return m_scrollOffset;
    }

    /**
     * @brief 是否还有未显示的更旧内容（用于标题栏提示可回看）
     */
    [[nodiscard]] bool hasHiddenOlderLines() const noexcept
    {
        return _contentLineCount() > m_scrollOffset + m_viewportRows;
    }

    /**
     * @brief 是否还有未显示的更新内容（回看中且已滚过的行）
     */
    [[nodiscard]] bool hasHiddenNewerLines() const noexcept
    {
        return m_scrollOffset > 0;
    }

    /**
     * @brief 刷新可见行缓存
     *
     * 只有日志/事件缓冲的累计计数变化时才需要重新取快照，避免每帧整份拷贝。
     */
    void refreshIfDirty();

    /**
     * @brief 计算每页可显示的行数（由布局在渲染时告知）
     * @param rows 可见区域高度
     */
    void setViewportRows(usize rows) noexcept
    {
        m_viewportRows = rows;
    }

    /**
     * @brief 设置折行宽度（由布局在渲染时告知，取当前 xflex 区域宽度）
     *
     * FTXUI 的 text() 不自动折行，超宽内容会被截断；日志必须在紧凑行上先折好。
     * 宽度变化会让折行缓存失效，下一帧重算。
     *
     * @param columns 每行最大显示宽度
     */
    void setWrapWidth(usize columns);

private:
    /**
     * @brief 回看时可用行的总数（折行后的显示行数）
     */
    [[nodiscard]] usize _contentLineCount() const noexcept
    {
        return m_wrappedLines.size();
    }

    void _scrollBy(i64 delta);
    void _scrollPage(i64 pages);
    void _scrollToBottom();

    /**
     * @brief 按当前折行宽度重建显示行
     *
     * @param preserveViewport 是否保持"距底部固定行数"的回看位置（内容/宽度变化时为真）
     */
    void _rewrap(bool preserveViewport);

    const LogRingBuffer& m_logs;
    const EventRingBuffer& m_events;

    u64 m_observedLogCount = 0;
    u64 m_observedEventCount = 0;

    /// 由日志与事件拼出的紧凑行（每个元素仍是一条完整日志/事件，供折行缓存复用）。
    std::vector<std::pair<TerminalEventKind, std::string>> m_compactLines;
    /// 紧凑行中前多少条来自日志（其后为事件，事件行着色）。
    usize m_compactLogLineCount = 0;
    /// 折行后的显示行：(是否事件行, 事件分类, 文本)。日志行的分类恒为 Info。
    std::vector<std::tuple<bool, TerminalEventKind, std::string>> m_wrappedLines;
    /// 折行缓存的紧凑行总数（与 m_compactLines 一致）；不相等说明紧凑行已变。
    usize m_wrappedFromCount = 0;
    /// 折行缓存使用的宽度。
    usize m_wrappedWidth = 0;

    bool m_following = true;
    usize m_scrollOffset = 0;
    usize m_viewportRows = 10;
    usize m_wrapWidth = 80;

    /// 日志区在屏幕上的包围盒（渲染时由 reflect 回填），用于滚轮命中判定。
    ftxui::Box m_box;
};

} // namespace mc::server::console
