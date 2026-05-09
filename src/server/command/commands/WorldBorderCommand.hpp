#pragma once

#include "server/command/ServerCommandSource.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"

namespace mc {
namespace command {

/**
 * @brief WorldBorderCommand - 控制世界边界
 *
 * 用法: /worldborder <set|center|damage|warning|get|add>
 * 权限: 2 (游戏管理员)
 */
class WorldBorderCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 setBorder(CommandContext<ServerCommandSource>& context);
    static i32 getBorder(CommandContext<ServerCommandSource>& context);
    static i32 setCenter(CommandContext<ServerCommandSource>& context);
    static i32 setDamageAmount(CommandContext<ServerCommandSource>& context);
    static i32 setDamageBuffer(CommandContext<ServerCommandSource>& context);
    static i32 setWarningTime(CommandContext<ServerCommandSource>& context);
    static i32 setWarningDistance(CommandContext<ServerCommandSource>& context);
    static i32 addBorder(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
