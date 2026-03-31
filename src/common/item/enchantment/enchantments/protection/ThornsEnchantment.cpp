#include "ThornsEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

bool ThornsEnchantment::shouldTrigger(i32 level, math::Random& random) {
    if (level <= 0) {
        return false;
    }
    f32 chance = getTriggerChance(level);
    return random.nextFloat() < chance;
}

i32 ThornsEnchantment::getThornsDamage(i32 level, math::Random& random) {
    // 基础伤害 0.5-1.5，每级增加 0.5-1.5
    // I: 0.5-1.5 (1-3)
    // II: 0.5-2.5 (1-5)
    // III: 0.5-3.5 (1-7)
    // 转换为半心：乘以 2
    i32 minDamage = 1;  // 0.5颗心
    i32 maxDamage = 1 + level * 2;  // 每级增加 1 颗心
    return minDamage + static_cast<i32>(random.nextFloat() * (maxDamage - minDamage));
}

} // namespace enchant
} // namespace item
} // namespace mc
