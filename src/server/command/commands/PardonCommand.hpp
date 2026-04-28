#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /pardon 命令
 *
 * 用法：
 * - /pardon <player> - 解封玩家
 *
 * 权限等级：3（需要管理员权限）
 *
 * 参考 MC 1.16.5 的 PardonCommand
 */
class PardonCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 pardonPlayer(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
