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

#include "GameModeUtils.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "entity/entities/player/Player.hpp"
#include <cstddef>

namespace mc {
namespace entity {

namespace {
// 游戏模式能力配置表
// 顺序: creativeMode, canFly, flying, invulnerable, allowEdit, flySpeed, walkSpeed
struct ModeConfig {
    bool creativeMode;
    bool canFly;
    bool flying;
    bool invulnerable;
    bool allowEdit;
    f32 flySpeed;
    f32 walkSpeed;
};

constexpr size_t MODE_COUNT = 4; // Survival, Creative, Adventure, Spectator

constexpr ModeConfig MODE_CONFIGS[] = {
    {false, false, false, false, true, physics::FLY_SPEED, physics::WALK_SPEED},  // Survival (0)
    {true, true, false, true, true, physics::FLY_SPEED, physics::WALK_SPEED},     // Creative (1)
    {false, false, false, false, false, physics::FLY_SPEED, physics::WALK_SPEED}, // Adventure (2)
    {false, true, true, true, false, physics::FLY_SPEED, physics::WALK_SPEED},    // Spectator (3)
};
} // namespace

PlayerAbilities GameModeUtils::getAbilitiesForGameMode(GameMode mode)
{
    PlayerAbilities abilities;

    size_t index = static_cast<size_t>(mode);
    if (index >= MODE_COUNT) {
        index = 0; // 默认为 Survival
    }

    const auto& config = MODE_CONFIGS[index];
    abilities.creativeMode = config.creativeMode;
    abilities.canFly = config.canFly;
    abilities.flying = config.flying;
    abilities.invulnerable = config.invulnerable;
    abilities.allowEdit = config.allowEdit;
    abilities.flySpeed = config.flySpeed;
    abilities.walkSpeed = config.walkSpeed;

    return abilities;
}

bool GameModeUtils::canFly(GameMode mode)
{
    return mode == GameMode::Creative || mode == GameMode::Spectator;
}

bool GameModeUtils::isInvulnerable(GameMode mode)
{
    return mode == GameMode::Creative || mode == GameMode::Spectator;
}

bool GameModeUtils::canEdit(GameMode mode)
{
    return mode != GameMode::Adventure && mode != GameMode::Spectator;
}

bool GameModeUtils::isBlockPlacingRestricted(GameMode mode)
{
    return mode == GameMode::Adventure || mode == GameMode::Spectator;
}

bool GameModeUtils::isCreative(GameMode mode)
{
    return mode == GameMode::Creative;
}

bool GameModeUtils::isSurvival(GameMode mode)
{
    return mode == GameMode::Survival;
}

bool GameModeUtils::isAdventure(GameMode mode)
{
    return mode == GameMode::Adventure;
}

bool GameModeUtils::isSpectator(GameMode mode)
{
    return mode == GameMode::Spectator;
}

} // namespace entity
} // namespace mc
