#include "RaiderType.hpp"

namespace mc {
namespace world {
namespace village {
namespace raid {

// ============================================================================
// RaiderTypeHelper 实现
// ============================================================================

const char* RaiderTypeHelper::getName(RaiderType type) {
    switch (type) {
        case RaiderType::Pillager:   return "Pillager";
        case RaiderType::Vindicator: return "Vindicator";
        case RaiderType::Evoker:     return "Evoker";
        case RaiderType::Ravager:    return "Ravager";
        case RaiderType::Witch:      return "Witch";
        default:                     return "Unknown";
    }
}

f32 RaiderTypeHelper::getBaseHealth(RaiderType type) {
    switch (type) {
        case RaiderType::Pillager:   return 24.0f;
        case RaiderType::Vindicator: return 24.0f;
        case RaiderType::Evoker:     return 24.0f;
        case RaiderType::Ravager:    return 100.0f;
        case RaiderType::Witch:      return 26.0f;
        default:                     return 20.0f;
    }
}

i32 RaiderTypeHelper::getSpawnWeight(RaiderType type, i32 wave) {
    // 根据波次调整生成权重
    // 后期波次增加更强的敌人权重
    switch (type) {
        case RaiderType::Pillager:
            return 10; // 始终常见

        case RaiderType::Vindicator:
            return wave >= 3 ? 5 : 0; // 第3波后出现

        case RaiderType::Evoker:
            return wave >= 5 ? 1 : 0; // 第5波后出现

        case RaiderType::Ravager:
            return wave >= 6 ? 1 : 0; // 第6波后出现

        case RaiderType::Witch:
            return wave >= 4 ? 2 : 0; // 第4波后出现

        default:
            return 0;
    }
}

bool RaiderTypeHelper::canRideRavager(RaiderType type) {
    // 掠夺者和灾厄村民可以骑劫掠兽
    switch (type) {
        case RaiderType::Pillager:
        case RaiderType::Vindicator:
        case RaiderType::Evoker:
            return true;
        default:
            return false;
    }
}

} // namespace raid
} // namespace village
} // namespace world
} // namespace mc
