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
 * @brief ScoreboardCommand - 记分板管理
 *
 * 用法: /scoreboard <objectives|players|teams> ...
 * 权限: 2 (游戏管理员)
 */
class ScoreboardCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 _addObjective(CommandContext<ServerCommandSource>& context);
    static i32 _removeObjective(CommandContext<ServerCommandSource>& context);
    static i32 _listObjectives(CommandContext<ServerCommandSource>& context);
    static i32 _setScore(CommandContext<ServerCommandSource>& context);
    static i32 _addScore(CommandContext<ServerCommandSource>& context);
    static i32 _removeScore(CommandContext<ServerCommandSource>& context);
    static i32 _resetScore(CommandContext<ServerCommandSource>& context);
    static i32 _getScore(CommandContext<ServerCommandSource>& context);
    static i32 _enableTrigger(CommandContext<ServerCommandSource>& context);
    static i32 _listPlayers(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
