#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc::command {

/**
 * @brief /stop 命令。
 */
class StopCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 stop(CommandContext<ServerCommandSource>& context);
};

} // namespace mc::command
