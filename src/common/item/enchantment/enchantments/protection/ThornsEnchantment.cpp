#include "ThornsEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

bool ThornsEnchantment::shouldTrigger(i32 level, math::Random& random) {
    if (level <= 0) {
        return false;
    }
    // MC 1.16.5: 每级 15% 概率触发
    // shouldHit(int level, Random rnd) { return level > 0 && rnd.nextFloat() < 0.15F * level; }
    return random.nextFloat() < static_cast<f32>(level) * 0.15f;
}

i32 ThornsEnchantment::getThornsDamage(i32 level, math::Random& random) {
    // MC 1.16.5: getDamage(int level, Random rnd)
    // return level > 10 ? level - 10 : 1 + rnd.nextInt(4);
    // 等级 > 10 时返回 level - 10，否则返回 1-4
    if (level > 10) {
        return level - 10;
    }
    return 1 + random.nextInt(4);
}

} // namespace enchant
} // namespace item
} // namespace mc
