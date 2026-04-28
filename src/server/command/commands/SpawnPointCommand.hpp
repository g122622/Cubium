#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandSource.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief /spawnpoint 命令
 *
 * 设置玩家的重生点。
 * 权限等级: 2 (OP)
 *
 * 用法:
 * - /spawnpoint - 设置命令执行者的重生点到当前位置
 * - /spawnpoint <player> - 设置指定玩家的重生点到其当前位置
 * - /spawnpoint <player> <pos> - 设置指定玩家的重生点到指定位置
 */
class SpawnPointCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 设置自己的重生点（当前位置）
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 setSelfSpawnPoint(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置指定玩家的重生点（玩家当前位置）
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 setPlayerSpawnPoint(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置指定玩家到指定位置的重生点
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 setPlayerSpawnPointAtPosition(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
