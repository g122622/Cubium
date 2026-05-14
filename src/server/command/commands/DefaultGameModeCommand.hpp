#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc::command {

/**
 * @brief /defaultgamemode 命令。
 */
class DefaultGameModeCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 setDefaultMode(CommandContext<ServerCommandSource>& context);
};

} // namespace mc::command
