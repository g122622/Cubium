#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "server/command/ServerCommandSource.hpp"

#include <memory>

namespace mc {
namespace command {

/**
 * @brief `/tp` 与 `/teleport` 命令。
 *
 * 当前命令实现优先对齐项目现有服务端主路径，只覆盖已经有稳定底层支撑的
 * 玩家传送能力，并刻意避免提前引入朝向、相对坐标、维度切换等尚未完成的
 * 子系统语义。
 *
 * 已支持的形式：
 * - `/tp <x> <y> <z>`
 * - `/tp <destinationPlayer>`
 * - `/tp <targets> <x> <y> <z>`
 * - `/tp <targets> <destinationPlayer>`
 *
 * @note `teleport` 为 `tp` 的别名重定向。
 */
class TeleportCommand {
public:
    /**
     * @brief 将命令注册到分发器。
     *
     * @param dispatcher 命令分发器。
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 将命令源传送到目标玩家位置。
     *
     * @param context 命令上下文。
     * @return 成功时返回 `1`，失败时返回 `0`。
     */
    static i32 teleportToEntity(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 将命令源传送到目标坐标。
     *
     * @param context 命令上下文。
     * @return 成功时返回 `1`，失败时返回 `0`。
     */
    static i32 teleportToPosition(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 将目标玩家集合传送到目标玩家位置。
     *
     * @param context 命令上下文。
     * @return 成功传送的玩家数量。
     */
    static i32 teleportTargetToEntity(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 将目标玩家集合传送到指定坐标。
     *
     * @param context 命令上下文。
     * @return 成功传送的玩家数量。
     */
    static i32 teleportTargetToPosition(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
