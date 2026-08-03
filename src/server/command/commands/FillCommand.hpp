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

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /fill 命令
 *
 * 用法：
 * - /fill <from> <to> <block> - 填充区域
 * - /fill <from> <to> <block> destroy - 破坏并填充
 * - /fill <from> <to> <block> hollow - 空心填充（仅外壳）
 * - /fill <from> <to> <block> keep - 仅替换空气
 * - /fill <from> <to> <block> outline - 轮廓填充（仅外壳）
 * - /fill <from> <to> <block> replace [filter] - 替换指定方块
 *
 * 权限等级：2
 */
class FillCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 _fill(CommandContext<ServerCommandSource>& context);
    static i32 _fillDestroy(CommandContext<ServerCommandSource>& context);
    static i32 _fillHollow(CommandContext<ServerCommandSource>& context);
    static i32 _fillKeep(CommandContext<ServerCommandSource>& context);
    static i32 _fillOutline(CommandContext<ServerCommandSource>& context);
    static i32 _fillReplace(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
