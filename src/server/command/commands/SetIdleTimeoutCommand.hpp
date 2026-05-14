#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc::command {

/**
 * @brief /setidletimeout 命令。
 */
class SetIdleTimeoutCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 setTimeout(CommandContext<ServerCommandSource>& context);
};

} // namespace mc::command
