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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "ArrowItem.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ProjectileItem.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/enchantments/AllEnchantments.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include <memory>

namespace mc {
namespace item {

ArrowItem::ArrowItem(const ItemProperties& properties)
    : Item(properties)
    , ProjectileItem()
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

std::unique_ptr<entity::ProjectileEntity> ArrowItem::asProjectile(IWorld& world,
    const Vector3& position,
    const ItemStack& /*stack*/,
    f32 /*directionX*/,
    f32 /*directionY*/,
    f32 /*directionZ*/) const
{
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = world.entityRegistry();
    if (registry == nullptr) {
        return nullptr;
    }
    auto entity = entity::ArrowEntity::create(&world, *registry);
    if (entity) {
        entity->setPosition(position.x, position.y, position.z);
        // 发射器发射的箭矢允许被玩家拾取
        auto* arrow = dynamic_cast<entity::ArrowEntity*>(entity.get());
        if (arrow) {
            arrow->setPickupStatus(entity::PickupStatus::Allowed);
        }
        entity->setTypeId(entity::EntityTypeKeys::ARROW);
    }
    // ArrowEntity 继承自 ProjectileEntity，安全的 unique_ptr 转换
    return std::unique_ptr<entity::ProjectileEntity>(static_cast<entity::ProjectileEntity*>(entity.release()));
}

ProjectileDispenseConfig ArrowItem::getDispenseConfig() const
{
    return ProjectileDispenseConfig::arrow();
}

} // namespace item
} // namespace mc
