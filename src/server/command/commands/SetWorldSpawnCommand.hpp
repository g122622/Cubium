#pragma once

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandSource.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace command {

/**
 * @brief /setworldspawn 命令
 *
 * 设置世界的出生点（新玩家进入世界的初始位置）。
 * 权限等级: 2 (OP)
 *
 * 用法:
 * - /setworldspawn - 设置世界出生点到当前位置
 * - /setworldspawn <pos> - 设置世界出生点到指定位置
 */
class SetWorldSpawnCommand {
public:
    /**
     * @brief 注册命令到分发器
     * @param dispatcher 命令分发器
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 设置世界出生点到当前位置
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 setCurrentPosition(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置世界出生点到指定位置
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 setPosition(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
