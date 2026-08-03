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
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
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
    static i32 _teleportToEntity(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 将命令源传送到目标坐标。
     *
     * @param context 命令上下文。
     * @return 成功时返回 `1`，失败时返回 `0`。
     */
    static i32 _teleportToPosition(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 将目标玩家集合传送到目标玩家位置。
     *
     * @param context 命令上下文。
     * @return 成功传送的玩家数量。
     */
    static i32 _teleportTargetToEntity(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 将目标玩家集合传送到指定坐标。
     *
     * @param context 命令上下文。
     * @return 成功传送的玩家数量。
     */
    static i32 _teleportTargetToPosition(CommandContext<ServerCommandSource>& context);
};

} // namespace command
} // namespace mc
