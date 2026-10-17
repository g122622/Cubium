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
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc {
namespace server {
class IServer;
}

namespace command {

/**
 * @brief /setworldspawn 命令
 *
 * 设置世界的出生点（新玩家进入世界的初始位置）。
 * 权限等级: 2 (OP)
 *
 * 用法（对齐 MC 1.21.11 SetWorldSpawnCommand）:
 * - /setworldspawn - 设置世界出生点到当前位置（floor 成 BlockPos），朝向 ZERO_ROTATION（0,0）
 * - /setworldspawn <pos> - 设置世界出生点到指定 BlockPos，朝向 ZERO_ROTATION
 * - /setworldspawn <pos> <rotation> - 设置世界出生点到指定 BlockPos 和朝向（yaw pitch）
 *
 * 对齐要点（曾为偏差，已修）:
 * - pos 参数用 BlockPosArgumentType（整数 floor，对齐 vanilla BlockPosArgument），
 *   非 Vec3ArgumentType（centerCorrect 会给绝对整数加 0.5 偏移到方块中心，导致出生点偏 0.5）。
 * - 无参分支 pos = BlockPos.containing(source.position())（floor），rotation = ZERO_ROTATION（0,0），
 *   非"玩家当前朝向"（vanilla 无参不用玩家朝向）。
 * - <pos> 分支 rotation = ZERO_ROTATION（0,0），非 angle=0（语义等价但显式对齐）。
 * - <rotation> 用 RotationArgumentType（接 yaw pitch 两值，对齐 vanilla RotationArgument）；
 *   yaw/pitch 一起写入 level.dat 的 spawn 复合标签并随 SetDefaultSpawnPosition 下发。
 *
 * 参考 MC 1.21.11: SetWorldSpawnCommand
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
     * @brief 设置世界出生点到当前位置（对齐 vanilla BlockPos.containing + ZERO_ROTATION）
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 _setCurrentPosition(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置世界出生点到指定 BlockPos（对齐 vanilla BlockPosArgument + ZERO_ROTATION）
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 _setPosition(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 设置世界出生点到指定 BlockPos 和朝向（对齐 vanilla BlockPosArgument + RotationArgument）
     *
     * rotation 解析为 (yaw, pitch)，随世界出生数据一起持久化并广播。
     * @param context 命令上下文
     * @return 命令结果
     */
    static i32 _setPositionWithRotation(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 应用新的世界出生点：写入主世界出生数据并向所有玩家广播
     *
     * @param context 命令上下文
     * @param position 出生位置（世界坐标）
     * @param rotation 朝向（yaw, pitch）
     * @return 出生点实际变化返回 1，否则返回 0
     */
    static i32 _applyWorldSpawn(
        CommandContext<ServerCommandSource>& context, const Vector3d& position, const Vector2f& rotation);

    /**
     * @brief 广播新的世界出生点到所有玩家
     * @param server 服务器实例
     * @param pos 新的出生点位置
     * @param angle 新的出生点朝向（度）
     * @param dimensionId 出生点所在维度
     */
};

} // namespace command
} // namespace mc
