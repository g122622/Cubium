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

#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"
#include <string>
#include <vector>

namespace mc::command::support {

/**
 * @brief 解析单个玩家选择器。
 *
 * @param source 命令源
 * @param selector 玩家选择器
 * @return 匹配到的玩家 ID，失败时返回 0
 * @note 当前仅落地与现有项目能力直接兼容的玩家解析语义。
 */
[[nodiscard]] PlayerId resolveSinglePlayerId(const ServerCommandSource& source, const EntitySelector& selector);

/**
 * @brief 解析多个玩家选择器。
 *
 * @param source 命令源
 * @param selector 玩家选择器
 * @return 匹配到的玩家 ID 列表
 */
[[nodiscard]] std::vector<PlayerId> resolvePlayerIds(const ServerCommandSource& source, const EntitySelector& selector);

/**
 * @brief 通过 PlayerId 获取玩家名称。
 *
 * 查找 PlayerManager 中对应 PlayerId 的 ServerPlayerData，返回其 username。
 * 如果服务器不可用或玩家不在线，返回回退名称 "player_<id>"。
 *
 * @param source 命令源，用于获取服务器实例
 * @param playerId 玩家 ID
 * @return 玩家名称字符串
 */
[[nodiscard]] std::string resolvePlayerName(const ServerCommandSource& source, PlayerId playerId);

/**
 * @brief 将游戏模式转换为命令输出名称。
 */
[[nodiscard]] const char* getGameModeCommandName(GameMode mode) noexcept;

/**
 * @brief 将难度转换为命令输出名称。
 */
[[nodiscard]] const char* getDifficultyCommandName(Difficulty difficulty) noexcept;

} // namespace mc::command::support
