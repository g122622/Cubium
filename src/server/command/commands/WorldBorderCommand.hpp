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

namespace mc {
namespace command {

/**
 * @brief WorldBorderCommand - 控制世界边界
 *
 * 用法: /worldborder <set|center|damage|warning|get|add>
 * 权限: 2 (游戏管理员)
 */
class WorldBorderCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 _setBorder(CommandContext<ServerCommandSource>& context);
    static i32 _getBorder(CommandContext<ServerCommandSource>& context);
    static i32 _setCenter(CommandContext<ServerCommandSource>& context);
    static i32 _setDamageAmount(CommandContext<ServerCommandSource>& context);
    static i32 _setDamageBuffer(CommandContext<ServerCommandSource>& context);
    static i32 _setWarningTime(CommandContext<ServerCommandSource>& context);
    static i32 _setWarningDistance(CommandContext<ServerCommandSource>& context);
    static i32 _addBorder(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
