#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc::command {

/**
 * @brief /say 命令。
 */
class SayCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 say(CommandContext<ServerCommandSource>& context);
};

} // namespace mc::command
