#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief TriggerCommand - 触发记分板目标
 *
 * 用法: /trigger <objective> [add|set] [value]
 * 权限: 0 (所有玩家)
 */
class TriggerCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 triggerAdd(CommandContext<ServerCommandSource>& context);
    static i32 triggerSet(CommandContext<ServerCommandSource>& context);
    static i32 trigger(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
