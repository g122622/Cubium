#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /op 命令
 *
 * 用法：
 * - /op <player> - 授予玩家 OP 权限
 *
 * 权限等级：3（需要管理员权限）
 *
 * 参考 MC 1.16.5 的 OpCommand
 */
class OpCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 opPlayer(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
