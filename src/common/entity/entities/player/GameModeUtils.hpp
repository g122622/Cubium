#pragma once

#include "common/core/Types.hpp"

namespace mc {

// Forward declaration
struct PlayerAbilities;

namespace entity {

/**
 * @brief 游戏模式工具函数
 *
 * 提供游戏模式相关的能力计算工具。
 * 参考 MC 1.16.5 PlayerList#setGameMode
 */
namespace GameModeUtils {

/// 根据游戏模式计算玩家能力
[[nodiscard]] PlayerAbilities getAbilitiesForGameMode(GameMode mode);

/// 创造模式和旁观者模式允许飞行
[[nodiscard]] bool canFly(GameMode mode);

/// 创造模式和旁观者模式无敌
[[nodiscard]] bool isInvulnerable(GameMode mode);

/// 冒险模式和旁观者模式不允许编辑
[[nodiscard]] bool canEdit(GameMode mode);

[[nodiscard]] bool isCreative(GameMode mode);
[[nodiscard]] bool isSurvival(GameMode mode);
[[nodiscard]] bool isAdventure(GameMode mode);
[[nodiscard]] bool isSpectator(GameMode mode);

} // namespace GameModeUtils
} // namespace entity
} // namespace mc
