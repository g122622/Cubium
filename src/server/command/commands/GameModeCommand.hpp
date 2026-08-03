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
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"

#include <memory>

namespace mc {
namespace command {

/**
 * @brief `/gamemode` 命令。
 *
 * 该命令负责修改命令源玩家或指定玩家集合的游戏模式。
 * 当前实现直接依赖项目现有的 `GameModeManager` 与 `ServerPlayerData`，
 * 避免把命令层耦合到尚未完成接入的 `ServerPlayer` 运行时层。
 *
 * 已支持的形式：
 * - `/gamemode <mode>`
 * - `/gamemode <mode> <target>`
 *
 * @note `target` 当前仅解析玩家选择器语义。
 */
class GameModeCommand {
public:
    /**
     * @brief 将命令注册到分发器。
     *
     * @param dispatcher 命令分发器。
     */
    static void registerTo(CommandDispatcher<ServerCommandSource>& dispatcher);

private:
    /**
     * @brief 将命令源玩家切换到指定游戏模式。
     *
     * @param context 命令上下文。
     * @return 成功时返回 `1`，失败时返回 `0`。
     *
     * @warning 该分支要求命令源必须是玩家。
     */
    static i32 _setGameModeSelf(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 将目标玩家集合切换到指定游戏模式。
     *
     * @param context 命令上下文。
     * @return 成功修改的玩家数量。
     *
     * @note 选择器解析统一走 `support::resolvePlayerIds()`，避免命令各自复制解析逻辑。
     */
    static i32 _setGameModeOthers(CommandContext<ServerCommandSource>& context);

    /**
     * @brief 获取命令反馈中使用的游戏模式名称。
     *
     * @param mode 游戏模式。
     * @return 可读名称。
     */
    static const char* _getGameModeName(GameMode mode);
};

} // namespace command
} // namespace mc
