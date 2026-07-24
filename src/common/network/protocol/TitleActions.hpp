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

namespace mc::network {

/// @brief 标题动作类型
///
/// 1.21.11 已将标题拆为 5 个独立包（SetTitleText/SetSubtitleText/SetActionBarText/
/// SetTitlesAnimation/ClearTitles）。此枚举保留作为客户端 TitleWidget 统一入口的
/// 内部分发标签：ClientPlayVisitor 把 5 个 IR 包映射回 TitleAction 后调
/// TitleWidget::handleTitlePacket。
enum class TitleAction : u8 {
    Title = 0,     // 设置主标题
    Subtitle = 1,  // 设置副标题
    Actionbar = 2, // 设置动作栏
    Times = 3,     // 设置时间参数
    Clear = 4,     // 清除标题
    Reset = 5      // 重置标题（清除并重置时间参数）
};

} // namespace mc::network
