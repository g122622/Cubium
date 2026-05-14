#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief MeCommand - 在聊天中显示动作消息
 *
 * 用法: /me <action>
 * 权限: 所有玩家
 */
class MeCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 performAction(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
