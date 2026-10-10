#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc::command {
/** @brief /dialog show，权限等级 2，支持命名对话框和 JSON 直接定义。 */
class DialogCommand {
public:
    /** @brief 注册对话框命令。 */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);
};
} // namespace mc::command
