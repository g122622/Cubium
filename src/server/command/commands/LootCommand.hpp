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
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <vector>

namespace mc {
namespace command {

/**
 * @brief /loot 命令 - 从战利品表生成物品
 *
 * 完整语法：
 *
 * 源（从哪里获取战利品）：
 *   loot <loot_table>           - 直接从战利品表获取
 *   fish <loot_table> <pos> [tool|mainhand|offhand] - 钓鱼
 *   kill <target>               - 实体死亡掉落
 *   mine <pos> [tool|mainhand|offhand] - 挖掘方块掉落
 *
 * 目标（战利品放到哪里）：
 *   give <players>              - 给予玩家
 *   spawn <targetPos>           - 生成物品实体
 *   insert <targetPos>          - 插入容器
 *   replace entity <entities> <slot> [count] - 替换实体槽位
 *   replace block <targetPos> <slot> [count]  - 替换容器槽位
 *
 * 权限等级：2（游戏管理员）
 */
class LootCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    // ========== /loot loot <loot_table> → 目标 ==========

    static i32 lootGive(CommandContext<ServerCommandSource>& context);
    static i32 lootSpawn(CommandContext<ServerCommandSource>& context);
    static i32 lootInsert(CommandContext<ServerCommandSource>& context);
    static i32 lootReplaceEntity(CommandContext<ServerCommandSource>& context);
    static i32 lootReplaceBlock(CommandContext<ServerCommandSource>& context);

    // ========== /loot fish <loot_table> <pos> [tool] → 目标 ==========

    static i32 fishGive(CommandContext<ServerCommandSource>& context);
    static i32 fishSpawn(CommandContext<ServerCommandSource>& context);
    static i32 fishInsert(CommandContext<ServerCommandSource>& context);
    static i32 fishReplaceEntity(CommandContext<ServerCommandSource>& context);
    static i32 fishReplaceBlock(CommandContext<ServerCommandSource>& context);

    // ========== /loot kill <target> → 目标 ==========

    static i32 killGive(CommandContext<ServerCommandSource>& context);
    static i32 killSpawn(CommandContext<ServerCommandSource>& context);
    static i32 killInsert(CommandContext<ServerCommandSource>& context);
    static i32 killReplaceEntity(CommandContext<ServerCommandSource>& context);

    // ========== /loot mine <pos> [tool] → 目标 ==========

    static i32 mineGive(CommandContext<ServerCommandSource>& context);
    static i32 mineSpawn(CommandContext<ServerCommandSource>& context);
    static i32 mineInsert(CommandContext<ServerCommandSource>& context);
    static i32 mineReplaceBlock(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
