#pragma once

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc::command {

/**
 * @brief /difficulty 命令。
 */
class DifficultyCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 queryDifficulty(CommandContext<ServerCommandSource>& context);
    static i32 setDifficulty(CommandContext<ServerCommandSource>& context);
};

} // namespace mc::command