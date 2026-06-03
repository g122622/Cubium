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

#include "ArrowItem.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/enchantments/AllEnchantments.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace item {

ArrowItem::ArrowItem(const ItemProperties& properties)
    : Item(properties)
{}

entity::AbstractArrowEntity* ArrowItem::createArrow(IWorld& world, const ItemStack& stack, LivingEntity& shooter) const
{
    (void)stack; // 普通箭不使用物品堆信息

    // 使用ArrowEntity的工厂方法创建箭矢
    auto arrow = entity::ArrowEntity::createFromShooter(shooter, &world);
    if (arrow) {
        return arrow.release(); // 释放所有权，返回裸指针
    }
    return nullptr;
}

bool ArrowItem::isInfinite(const ItemStack& arrowStack, const ItemStack& bowStack, Player& player) const
{
    // 创造模式总是无限
    if (player.isCreative()) {
        return true;
    }

    // 检查弓是否有无限附魔
    i32 infinityLevel =
        enchant::EnchantmentHelper::getEnchantmentLevel(bowStack, &enchant::AllEnchantments::INFINITY_ARROW);

    if (infinityLevel <= 0) {
        return false;
    }

    // 只有普通箭（ArrowItem类）受益于无限附魔
    // 光灵箭和药水箭需要子类重写此方法返回false
    (void)arrowStack; // 普通箭不检查物品堆类型
    return true;
}

} // namespace item
} // namespace mc
