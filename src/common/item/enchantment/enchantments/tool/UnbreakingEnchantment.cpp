#include "UnbreakingEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

bool UnbreakingEnchantment::shouldConsumeDurability(i32 level, math::Random& random) {
    if (level <= 0) {
        return true;
    }
    // 有 (level + 1) / (level + 50) 的概率不消耗耐久
    f32 chance = static_cast<f32>(level + 1) / static_cast<f32>(level + 50);
    return random.nextFloat() >= chance;
}

bool UnbreakingEnchantment::shouldArmorConsumeDurability(i32 level, math::Random& random) {
    if (level <= 0) {
        return true;
    }
    // 盔甲有 (level + 1) / (level + 100) 的概率不消耗耐久
    f32 chance = static_cast<f32>(level + 1) / static_cast<f32>(level + 100);
    return random.nextFloat() >= chance;
}

} // namespace enchant
} // namespace item
} // namespace mc
