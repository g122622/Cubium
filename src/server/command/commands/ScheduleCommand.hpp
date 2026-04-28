#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief ScheduleCommand - 延迟执行函数或命令
 *
 * 用法: /schedule function <function> <time> [append|replace]
 *       /schedule clear <function>
 * 权限: 2 (游戏管理员)
 */
class ScheduleCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 scheduleFunction(CommandContext<ServerCommandSource>& context, bool append);
    static i32 clearSchedule(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
