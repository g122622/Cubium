#include "TieredItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../crafting/Ingredient.hpp"

namespace mc {
namespace item {
namespace tool {

TieredItem::TieredItem(const tier::IItemTier& tier, ItemProperties properties)
    : Item(properties.maxDamage(tier.getMaxUses()))
    , m_tier(tier) {
}

bool TieredItem::isRepairable(const ItemStack& toRepair, const ItemStack& repair) const {
    // 检查修复材料是否匹配层级的修复材料
    const auto& repairMaterial = m_tier.getRepairMaterial();
    return repairMaterial.test(repair);
}

} // namespace tool
} // namespace item
} // namespace mc
