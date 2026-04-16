#pragma once

#include "common/command/arguments/EntityArgument.hpp"
#include "server/command/ServerCommandSource.hpp"
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
[[nodiscard]] PlayerId resolveSinglePlayerId(
    const ServerCommandSource& source,
    const EntitySelector& selector
);

/**
 * @brief 解析多个玩家选择器。
 *
 * @param source 命令源
 * @param selector 玩家选择器
 * @return 匹配到的玩家 ID 列表
 */
[[nodiscard]] std::vector<PlayerId> resolvePlayerIds(
    const ServerCommandSource& source,
    const EntitySelector& selector
);

/**
 * @brief 将游戏模式转换为命令输出名称。
 */
[[nodiscard]] const char* getGameModeCommandName(GameMode mode) noexcept;

/**
 * @brief 将难度转换为命令输出名称。
 */
[[nodiscard]] const char* getDifficultyCommandName(Difficulty difficulty) noexcept;

} // namespace mc::command::support
