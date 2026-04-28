#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief TeamCommand - 队伍管理
 *
 * 用法: /team <add|remove|list|empty|join|leave|modify> ...
 * 权限: 2 (游戏管理员)
 */
class TeamCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 addTeam(CommandContext<ServerCommandSource>& context);
    static i32 removeTeam(CommandContext<ServerCommandSource>& context);
    static i32 listTeams(CommandContext<ServerCommandSource>& context);
    static i32 emptyTeam(CommandContext<ServerCommandSource>& context);
    static i32 joinTeam(CommandContext<ServerCommandSource>& context);
    static i32 leaveTeam(CommandContext<ServerCommandSource>& context);
    static i32 modifyTeam(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
