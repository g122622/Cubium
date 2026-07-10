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
#include "client/ui/kagero/widget/TextFieldWidget.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <array>
#include <functional>
#include <string>

namespace mc::client::ui::minecraft {

/**
 * @brief 告示牌编辑屏幕
 *
 * 对应 MC Java 的 AbstractSignEditScreen。
 * 当玩家右键点击可编辑的告示牌时，服务端发送 OpenSignEditorPacket，
 * 客户端收到后弹出此屏幕，允许玩家编辑4行文本（每行最多15字符）。
 *
 * 交互：
 * - Tab/方向键在4个输入框之间切换焦点
 * - Enter 确认提交，ESC 取消
 * - 关闭时通过回调将编辑后的文本发送给服务端
 */
class SignEditScreen : public Screen {
public:
    /// 告示牌行数（与 SignEntity::LINE_COUNT 一致）
    static constexpr i32 LINE_COUNT = 4;

    /// 每行最大字符数（与 SignEntity::MAX_LINE_LENGTH 一致）
    static constexpr i32 MAX_LINE_LENGTH = 15;

    /// 提交回调类型：参数为4行文本和是否正面
    using SubmitCallback = std::function<void(const BlockPos&, const std::array<std::string, LINE_COUNT>&, bool)>;

    /// 关闭回调类型（无参数，用于通知屏幕栈弹出此屏幕）
    using CloseCallback = std::function<void()>;

    /**
     * @brief 构造告示牌编辑屏幕
     * @param pos 告示牌方块位置
     * @param initialLines 初始4行文本
     * @param isFrontSide 是否编辑正面
     * @param submitCallback 提交回调（发送更新包给服务端）
     * @param closeCallback 关闭回调（弹出屏幕栈）
     */
    SignEditScreen(const BlockPos& pos,
        const std::array<std::string, LINE_COUNT>& initialLines,
        bool isFrontSide,
        SubmitCallback submitCallback,
        CloseCallback closeCallback);

    ~SignEditScreen() override = default;

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override;

    void paint(kagero::widget::PaintContext& ctx) override;

protected:
    void onOpen() override;
    void onClose() override;
    void tick(f32 dt) override;

private:
    /// 初始化4个文本输入框
    void _initTextFields();

    /// 提交编辑结果
    void _submit();

    /// 取消编辑
    void _cancel();

    // ========== 数据 ==========

    BlockPos m_pos;                                     ///< 告示牌方块位置
    std::array<std::string, LINE_COUNT> m_initialLines; ///< 初始文本
    bool m_isFrontSide;                                 ///< 是否正面
    SubmitCallback m_submitCallback;                    ///< 提交回调
    CloseCallback m_closeCallback;                      ///< 关闭回调

    // ========== UI 组件 ==========

    /// 4个文本输入框（按行顺序）
    std::array<kagero::widget::TextFieldWidget*, LINE_COUNT> m_textFields{};

    /// 当前焦点行索引
    i32 m_currentLine = 0;

    // ========== 布局常量 ==========

    static constexpr i32 SCREEN_PADDING = 20;
    static constexpr i32 TITLE_HEIGHT = 20;
    static constexpr i32 FIELD_HEIGHT = 18;
    static constexpr i32 FIELD_SPACING = 2;
    static constexpr i32 FIELD_WIDTH = 120;
};

} // namespace mc::client::ui::minecraft
