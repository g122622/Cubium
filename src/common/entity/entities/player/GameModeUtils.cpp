#include "GameModeUtils.hpp"
#include "Player.hpp"

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

constexpr size_t MODE_COUNT = 4;  // Survival, Creative, Adventure, Spectator

constexpr ModeConfig MODE_CONFIGS[] = {
    {false, false, false, false, true,  0.05f, 0.1f},   // Survival (0)
    {true,  true,  false, true,  true,  0.05f, 0.1f},   // Creative (1)
    {false, false, false, false, false, 0.05f, 0.1f},   // Adventure (2)
    {false, true,  true,  true,  false, 0.05f, 0.1f},   // Spectator (3)
};
} // namespace

PlayerAbilities GameModeUtils::getAbilitiesForGameMode(GameMode mode) {
    PlayerAbilities abilities;

    size_t index = static_cast<size_t>(mode);
    if (index >= MODE_COUNT) {
        index = 0;  // 默认为 Survival
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

bool GameModeUtils::canFly(GameMode mode) {
    return mode == GameMode::Creative || mode == GameMode::Spectator;
}

bool GameModeUtils::isInvulnerable(GameMode mode) {
    return mode == GameMode::Creative || mode == GameMode::Spectator;
}

bool GameModeUtils::canEdit(GameMode mode) {
    return mode != GameMode::Adventure && mode != GameMode::Spectator;
}

bool GameModeUtils::isCreative(GameMode mode) {
    return mode == GameMode::Creative;
}

bool GameModeUtils::isSurvival(GameMode mode) {
    return mode == GameMode::Survival;
}

bool GameModeUtils::isAdventure(GameMode mode) {
    return mode == GameMode::Adventure;
}

bool GameModeUtils::isSpectator(GameMode mode) {
    return mode == GameMode::Spectator;
}

} // namespace entity
} // namespace mc
