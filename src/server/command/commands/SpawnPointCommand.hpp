/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandSource.hpp"
#include "common/core/Types.hpp"
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
 * - /spawnpoint - 把命令执行者的重生点设为命令源位置，朝向全零
 * - /spawnpoint <targets> - 把目标玩家的重生点设为命令源位置，朝向全零
 * - /spawnpoint <targets> <pos> - 把目标玩家的重生点设为指定位置，朝向全零
 * - /spawnpoint <targets> <pos> <rotation> - 额外指定 yaw/pitch
 *
 * 所有分支都写入强制重生点（forced=true）：重生时不要求目标方块是床或重生锚，
 * 只要方块与其上方一格都可站立即可生效；维度取命令源所在维度。
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
    static i32 _setSelfSpawnPoint(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置指定玩家的重生点（玩家当前位置）
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 _setPlayerSpawnPoint(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置指定玩家到指定位置的重生点
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 _setPlayerSpawnPointAtPosition(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置指定玩家到指定位置与朝向的重生点
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 _setPlayerSpawnPointAtRotation(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
