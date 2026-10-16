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

#include "item/crafting/special/FireworkRocketRecipe.hpp"

#include "common/core/Types.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/component/Fireworks.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/SpecialRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/Items.hpp"
#include <utility>
#include <vector>

namespace mc {
namespace crafting {

namespace {
/// vanilla FireworkRocketRecipe 的火药数量上限（决定飞行时长，最多 3）
constexpr i32 MAX_GUNPOWDER = 3;
} // namespace

FireworkRocketRecipe::FireworkRocketRecipe(const ResourceLocation& id)
    : SpecialRecipe(id)
{}

bool FireworkRocketRecipe::matches(const CraftingInventory& inventory) const
{
    i32 ingredientCount = 0;
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        if (!inventory.getItem(i).isEmpty()) {
            ++ingredientCount;
        }
    }
    if (ingredientCount < 2) {
        return false;
    }

    bool hasPaper = false;
    i32 gunpowderCount = 0;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        const ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }
        const Item* item = stack.getItem();

        if (item == Items::PAPER) {
            if (hasPaper) {
                return false; // 至多一张纸
            }
            hasPaper = true;
        } else if (item == Items::GUNPOWDER) {
            if (++gunpowderCount > MAX_GUNPOWDER) {
                return false; // 火药至多 3 个
            }
        } else if (item != Items::FIREWORK_STAR) {
            return false; // 除纸/火药/烟花之星外一律不匹配
        }
    }

    return hasPaper && gunpowderCount >= 1;
}

ItemStack FireworkRocketRecipe::assemble(const CraftingInventory& inventory) const
{
    std::vector<item::component::FireworkExplosion> explosions;
    i32 flightDuration = 0;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        const ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }
        const Item* item = stack.getItem();

        if (item == Items::GUNPOWDER) {
            ++flightDuration;
        } else if (item == Items::FIREWORK_STAR) {
            // 未携带 firework_explosion 的烟花之星不贡献爆炸（对齐 vanilla get() == null 分支）
            if (const auto* explosion = stack.getFireworkExplosion(); explosion != nullptr) {
                explosions.push_back(*explosion);
            }
        }
    }

    ItemStack result(Items::FIREWORK_ROCKET, 3);
    item::component::Fireworks fireworks{};
    fireworks.flightDuration = flightDuration;
    fireworks.explosions = std::move(explosions);
    result.setFireworks(std::move(fireworks));
    return result;
}

std::vector<ItemStack> FireworkRocketRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

} // namespace crafting
} // namespace mc
