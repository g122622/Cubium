#pragma once

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
    static i32 addObjective(CommandContext<ServerCommandSource>& context);
    static i32 removeObjective(CommandContext<ServerCommandSource>& context);
    static i32 listObjectives(CommandContext<ServerCommandSource>& context);
    static i32 setScore(CommandContext<ServerCommandSource>& context);
    static i32 addScore(CommandContext<ServerCommandSource>& context);
    static i32 removeScore(CommandContext<ServerCommandSource>& context);
    static i32 resetScore(CommandContext<ServerCommandSource>& context);
    static i32 getScore(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
