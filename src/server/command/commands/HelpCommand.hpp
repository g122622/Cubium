#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /help 命令。
 */
class HelpCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 showHelp(
        CommandContext<ServerCommandSource>& context, CommandDispatcher<ServerCommandSource>& dispatcher);
    static i32 showCommandHelp(
        CommandContext<ServerCommandSource>& context, CommandDispatcher<ServerCommandSource>& dispatcher);
};

} // namespace command
} // namespace mc
