#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief ReplaceItemCommand - 替换物品
 *
 * 用法: /replaceitem <entity|block> <target> <slot> <item> [count]
 * 权限: 2 (游戏管理员)
 */
class ReplaceItemCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 replaceEntityItem(CommandContext<ServerCommandSource>& context);
    static i32 replaceBlockItem(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
