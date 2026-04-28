#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief LootCommand - 战利品管理
 *
 * 用法: /loot <give|insert|replace|spawn> <target> <loot_table>
 * 权限: 2 (游戏管理员)
 */
class LootCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 giveLoot(CommandContext<ServerCommandSource>& context);
    static i32 insertLoot(CommandContext<ServerCommandSource>& context);
    static i32 replaceLoot(CommandContext<ServerCommandSource>& context);
    static i32 spawnLoot(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
