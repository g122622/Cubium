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
#include "common/command/CommandSource.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief /title 命令
 *
 * 控制玩家屏幕标题显示。
 * 权限等级: 2 (OP)
 *
 * 用法:
 * - /title <player> clear - 清除标题
 * - /title <player> reset - 重置标题
 * - /title <player> title <json> - 设置标题
 * - /title <player> subtitle <json> - 设置副标题
 * - /title <player> actionbar <json> - 设置动作栏
 * - /title <player> times <fadeIn> <stay> <fadeOut> - 设置时间
 */
class TitleCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 _clearTitle(CommandContext<ServerCommandSource>& context);
    static i32 _resetTitle(CommandContext<ServerCommandSource>& context);
    static i32 _setTitle(CommandContext<ServerCommandSource>& context);
    static i32 _setSubtitle(CommandContext<ServerCommandSource>& context);
    static i32 _setActionbar(CommandContext<ServerCommandSource>& context);
    static i32 _setTimes(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
