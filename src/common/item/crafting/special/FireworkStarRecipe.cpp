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

#include "item/crafting/special/FireworkStarRecipe.hpp"

#include "common/core/Types.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/component/FireworkExplosion.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/SpecialRecipe.hpp"
#include "common/item/items/DyeItemHelper.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/color/DyeColor.hpp"
#include "item/Items.hpp"
#include <unordered_map>
#include <vector>

namespace mc {
namespace crafting {

namespace {

/// 形状物品 → 爆炸形状（vanilla FireworkStarRecipe.SHAPE_BY_ITEM）
const std::unordered_map<const Item*, item::component::FireworkShape>& _shapeByItem()
{
    using item::component::FireworkShape;
    static const std::unordered_map<const Item*, FireworkShape> map = {
        {Items::FIRE_CHARGE, FireworkShape::LargeBall},
        {Items::FEATHER, FireworkShape::Burst},
        {Items::GOLD_NUGGET, FireworkShape::Star},
        {Items::SKELETON_SKULL, FireworkShape::Creeper},
        {Items::WITHER_SKELETON_SKULL, FireworkShape::Creeper},
        {Items::CREEPER_HEAD, FireworkShape::Creeper},
        {Items::PLAYER_HEAD, FireworkShape::Creeper},
        {Items::DRAGON_HEAD, FireworkShape::Creeper},
        {Items::ZOMBIE_HEAD, FireworkShape::Creeper},
        {Items::PIGLIN_HEAD, FireworkShape::Creeper},
    };
    return map;
}

/// 该物品是否为形状物品（对应 vanilla SHAPE_BY_ITEM.containsKey）
const Item* _shapeItem(const ItemStack& stack)
{
    const Item* item = stack.getItem();
    if (item == nullptr) {
        return nullptr;
    }
    return _shapeByItem().find(item) != _shapeByItem().end() ? item : nullptr;
}

} // namespace

FireworkStarRecipe::FireworkStarRecipe(const ResourceLocation& id)
    : SpecialRecipe(id)
{}

bool FireworkStarRecipe::matches(const CraftingInventory& inventory) const
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

    bool hasGunpowder = false;
    bool hasDye = false;
    bool hasShapeItem = false;
    bool hasTrail = false;
    bool hasTwinkle = false;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        const ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }
        const Item* item = stack.getItem();

        if (_shapeItem(stack) != nullptr) {
            if (hasShapeItem) {
                return false; // 至多一个形状物品
            }
            hasShapeItem = true;
        } else if (item == Items::GLOWSTONE_DUST) {
            if (hasTwinkle) {
                return false; // 至多一个荧石粉
            }
            hasTwinkle = true;
        } else if (item == Items::DIAMOND) {
            if (hasTrail) {
                return false; // 至多一个钻石
            }
            hasTrail = true;
        } else if (item == Items::GUNPOWDER) {
            if (hasGunpowder) {
                return false; // 至多一个火药
            }
            hasGunpowder = true;
        } else if (item::DyeItemHelper::isDyeItem(item)) {
            hasDye = true;
        } else {
            return false; // 其他物品一律不匹配
        }
    }

    return hasGunpowder && hasDye;
}

ItemStack FireworkStarRecipe::assemble(const CraftingInventory& inventory) const
{
    using item::component::FireworkExplosion;
    using item::component::FireworkShape;

    FireworkShape shape = FireworkShape::SmallBall;
    bool hasTwinkle = false;
    bool hasTrail = false;
    std::vector<i32> colors;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        const ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }
        const Item* item = stack.getItem();

        if (const Item* shapeItem = _shapeItem(stack); shapeItem != nullptr) {
            shape = _shapeByItem().at(shapeItem);
        } else if (item == Items::GLOWSTONE_DUST) {
            hasTwinkle = true;
        } else if (item == Items::DIAMOND) {
            hasTrail = true;
        } else if (const auto dyeColor = item::DyeItemHelper::dyeColorOfItem(item); dyeColor.has_value()) {
            colors.push_back(static_cast<i32>(dyeFireworkColor(*dyeColor)));
        }
    }

    ItemStack result(Items::FIREWORK_STAR, 1);
    FireworkExplosion explosion{};
    explosion.shape = shape;
    explosion.colors = std::move(colors);
    explosion.fadeColors = {};
    explosion.hasTrail = hasTrail;
    explosion.hasTwinkle = hasTwinkle;
    result.setFireworkExplosion(std::move(explosion));
    return result;
}

std::vector<ItemStack> FireworkStarRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

} // namespace crafting
} // namespace mc
