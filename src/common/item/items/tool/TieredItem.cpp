/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "TieredItem.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/item/tier/IItemTier.hpp"

namespace mc {
namespace item {
namespace tool {

TieredItem::TieredItem(const tier::IItemTier& tier, ItemProperties properties)
    : Item(properties.maxDamage(tier.getMaxUses()))
    , m_tier(tier)
{}

bool TieredItem::getIsRepairable(const ItemStack& toRepair, const ItemStack& repair) const
{
    // 检查修复材料是否匹配层级的修复材料
    (void)toRepair; // 工具修复不依赖于待修复物品的状态
    const auto& repairMaterial = m_tier.getRepairMaterial();
    return repairMaterial.test(repair);
}

} // namespace tool
} // namespace item
} // namespace mc
