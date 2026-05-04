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

bool TieredItem::getIsRepairable(const ItemStack& toRepair, const ItemStack& repair) const {
    // MC 1.16.5: 检查修复材料是否匹配层级的修复材料
    // 参考: net.minecraft.item.TieredItem#getIsRepairable
    (void)toRepair;  // 工具修复不依赖于待修复物品的状态
    const auto& repairMaterial = m_tier.getRepairMaterial();
    return repairMaterial.test(repair);
}

} // namespace tool
} // namespace item
} // namespace mc
