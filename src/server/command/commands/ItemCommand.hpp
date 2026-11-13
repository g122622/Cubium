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

namespace mc::command {

/**
 * @brief `/item` 命令（1.21.11 的 ItemCommands）。
 *
 * 用法：
 * - `/item replace block <pos> <slot> with <item> [<count>]`
 * - `/item replace block <pos> <slot> from block <source> <sourceSlot>`
 * - `/item replace block <pos> <slot> from entity <source> <sourceSlot>`
 * - `/item replace entity <targets> <slot> with <item> [<count>]`
 * - `/item replace entity <targets> <slot> from block|entity ...`
 *
 * 权限等级：2（游戏管理员）。
 *
 * TODO: `/item modify ...` 与 `from ... <modifier>` 需要战利品函数（LootItemFunction）注册表，
 * 该子系统尚未落地，因此这两条分支暂未注册。
 */
class ItemCommand {
public:
    /**
     * @brief 注册命令到分发器。
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /// `/item replace block <pos> <slot> with <item> [count]`
    static i32 _replaceBlockWith(CommandContext<ServerCommandSource>& context);

    /// `/item replace entity <targets> <slot> with <item> [count]`
    static i32 _replaceEntityWith(CommandContext<ServerCommandSource>& context);

    /// `/item replace block <pos> <slot> from <block|entity> ...`
    /// @param fromBlockSource 源是方块容器（true）还是实体槽位（false）
    static i32 _replaceBlockFrom(CommandContext<ServerCommandSource>& context, bool fromBlockSource);

    /// `/item replace entity <targets> <slot> from <block|entity> ...`
    /// @param fromBlockSource 源是方块容器（true）还是实体槽位（false）
    static i32 _replaceEntityFrom(CommandContext<ServerCommandSource>& context, bool fromBlockSource);
};

} // namespace mc::command
