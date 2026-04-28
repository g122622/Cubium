#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief ForceLoadCommand - 强制加载区块
 *
 * 用法: /forceload <add|remove|query> <pos> [to]
 * 权限: 2 (游戏管理员)
 */
class ForceLoadCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 addForceLoad(CommandContext<ServerCommandSource>& context);
    static i32 removeForceLoad(CommandContext<ServerCommandSource>& context);
    static i32 queryForceLoad(CommandContext<ServerCommandSource>& context);
    static i32 removeAllForceLoad(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
