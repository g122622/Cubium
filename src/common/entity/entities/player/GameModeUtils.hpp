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

#include "common/core/Types.hpp"

namespace mc {

// Forward declaration
struct PlayerAbilities;

namespace entity {

/**
 * @brief 游戏模式工具函数
 *
 * 提供游戏模式相关的能力计算工具。
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
