#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandSource.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief /enchant 命令
 *
 * 给玩家手持物品附魔。
 * 权限等级: 2 (OP)
 *
 * 用法:
 * - /enchant <player> <enchantment> [<level>]
 */
class EnchantCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 enchantItem(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
