#include "FrostWalkerEnchantment.hpp"
#include "DepthStriderEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

bool FrostWalkerEnchantment::isCompatibleWith(const Enchantment& other) const
{
    // 与深海探索者互斥
    if (other.id() == "minecraft:depth_strider") {
        return false;
    }
    return Enchantment::isCompatibleWith(other);
}

} // namespace enchant
} // namespace item
} // namespace mc
