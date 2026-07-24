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
#include "client/ui/kagero/widget/Widget.hpp"
#include "common/core/Types.hpp"
#include "common/network/protocol/TitleActions.hpp"

#include <optional>

namespace mc::client {
class Font;
}

namespace mc::client::ui::minecraft::widgets {

using namespace mc::network;

/**
 * @brief 标题显示 Widget
 *
 * 渲染游戏标题和副标题，支持：
 * - 主标题（屏幕中央大字）
 * - 副标题（标题下方小字）
 * - 动作栏（快捷栏上方，类似成就通知）
 * - 淡入/停留/淡出动画
 */
class TitleWidget : public kagero::widget::Widget {
public:
    TitleWidget();
    ~TitleWidget() override = default;

    // ========== 初始化 ==========

    /**
     * @brief 设置字体
     */
    void setFont(Font* font) { m_font = font; }

    // ========== 标题控制 ==========

    /**
     * @brief 设置主标题文本
     * @param text JSON 格式的文本组件
     */
    void setTitle(const std::string& text);

    /**
     * @brief 设置副标题文本
     * @param text JSON 格式的文本组件
     */
    void setSubtitle(const std::string& text);

    /**
     * @brief 设置动作栏文本
     * @param text JSON 格式的文本组件
     */
    void setActionbar(const std::string& text);

    /**
     * @brief 设置动画时间（以 tick 为单位，1 tick = 50ms）
     * @param fadeIn 淡入时间（tick）
     * @param stay 停留时间（tick）
     * @param fadeOut 淡出时间（tick）
     */
    void setTimes(i32 fadeIn, i32 stay, i32 fadeOut);

    /**
     * @brief 清除当前标题
     */
    void clear();

    /**
     * @brief 重置标题到默认状态
     */
    void reset();

    /**
     * @brief 处理标题包
     * @param action 标题动作
     * @param text 可选文本
     * @param fadeIn 淡入时间
     * @param stay 停留时间
     * @param fadeOut 淡出时间
     */
    void handleTitlePacket(
        TitleAction action, const std::optional<std::string>& text, i32 fadeIn, i32 stay, i32 fadeOut);

    // ========== Widget 接口 ==========

    void paint(kagero::widget::PaintContext& ctx) override;
    void tick(f32 dt) override;

private:
    // ========== 内部结构 ==========

    /**
     * @brief 标题状态
     */
    struct TitleState {
        std::optional<std::string> text; // 文本（JSON格式）
        f32 remainingTime = 0.0f;        // 剩余显示时间（秒）
        f32 fadeInTime = 0.0f;           // 淡入时间（秒）
        f32 stayTime = 0.0f;             // 停留时间（秒）
        f32 fadeOutTime = 0.0f;          // 淡出时间（秒）
        f32 elapsed = 0.0f;              // 已过时间（秒）
        bool active = false;             // 是否正在显示
    };

    // ========== 渲染方法 ==========

    /**
     * @brief 渲染标题（主标题 + 副标题）
     */
    void _renderTitle(kagero::widget::PaintContext& ctx);

    /**
     * @brief 渲染动作栏
     */
    void _renderActionbar(kagero::widget::PaintContext& ctx);

    /**
     * @brief 计算透明度
     * @param state 标题状态
     * @return 透明度 0.0-1.0
     */
    [[nodiscard]] f32 _calculateAlpha(const TitleState& state) const;

    // ========== 成员变量 ==========

    Font* m_font = nullptr; // TODO: 字体已设置但尚未在渲染中使用，需要接入字体渲染

    TitleState m_title;     // 主标题状态
    TitleState m_subtitle;  // 副标题状态
    TitleState m_actionbar; // 动作栏状态

    // 默认时间（以秒为单位：淡入10tick，停留70tick，淡出20tick）
    f32 m_defaultFadeIn = 0.5f;  // 10 ticks = 0.5 秒
    f32 m_defaultStay = 3.5f;    // 70 ticks = 3.5 秒
    f32 m_defaultFadeOut = 1.0f; // 20 ticks = 1.0 秒

    // 标题渲染常量
    static constexpr f32 TITLE_Y_RATIO = 0.25f;      // 标题Y位置比例
    static constexpr f32 ACTIONBAR_Y_RATIO = 0.85f;  // 动作栏Y位置比例
    static constexpr f32 TITLE_SHADOW_ALPHA = 0.25f; // 标题阴影透明度

    // 颜色常量
    static constexpr u32 TITLE_COLOR = 0xFFFFFFFF;  // 白色标题
    static constexpr u32 SHADOW_COLOR = 0xFF000000; // 黑色阴影
};

} // namespace mc::client::ui::minecraft::widgets
