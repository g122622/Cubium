#include "DepthStriderEnchantment.hpp"
#include "FrostWalkerEnchantment.hpp"

namespace mc {
namespace item {
namespace enchant {

bool DepthStriderEnchantment::isCompatibleWith(const Enchantment& other) const {
    // 与冰霜行者互斥
    if (other.id() == "minecraft:frost_walker") {
        return false;
    }
    return Enchantment::isCompatibleWith(other);
}

} // namespace enchant
} // namespace item
} // namespace mc
