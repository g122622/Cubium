#include "UnbreakingEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

bool UnbreakingEnchantment::shouldConsumeDurability(i32 level, math::Random& random) {
    if (level <= 0) {
        return true;
    }
    // MC 1.16.5: 有 level/(level+1) 的概率不消耗耐久
    // I: 50%, II: 67%, III: 75%
    f32 chance = static_cast<f32>(level) / static_cast<f32>(level + 1);
    return random.nextFloat() >= chance;
}

bool UnbreakingEnchantment::shouldArmorConsumeDurability(i32 level, math::Random& random) {
    if (level <= 0) {
        return true;
    }
    // MC 1.16.5: 盔甲有 60% 概率忽略耐久保护
    // 所以实际保护概率 = 0.4 * (level / (level + 1))
    f32 chance = 0.4f * static_cast<f32>(level) / static_cast<f32>(level + 1);
    return random.nextFloat() >= chance;
}

} // namespace enchant
} // namespace item
} // namespace mc
