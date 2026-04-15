#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <memory>

namespace mc {
namespace command {

/**
 * @brief /tp 命令
 *
 * 用法：
 * - /tp <target> - 传送到目标实体
 * - /tp <x> <y> <z> - 传送到坐标
 * - /tp <target> <destination> - 将目标传送到目的地
 * - /tp <target> <x> <y> <z> - 将目标传送到坐标
 * - /teleport ... - /tp 的重定向别名
 *
 * 权限等级：2
 *
 * 参考 MC 的 TeleportCommand
 */
class TeleportCommand {
public:
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    static i32 teleportToEntity(CommandContext<ServerCommandSource>& context);
    static i32 teleportToPosition(CommandContext<ServerCommandSource>& context);
    static i32 teleportTargetToEntity(CommandContext<ServerCommandSource>& context);
    static i32 teleportTargetToPosition(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
