#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief SpreadPlayersCommand - 随机分散玩家
 *
 * 用法: /spreadplayers <center> <spreadDistance> <maxRange> <respectTeams> <targets>
 * 权限: 2 (游戏管理员)
 */
class SpreadPlayersCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 spreadPlayers(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
