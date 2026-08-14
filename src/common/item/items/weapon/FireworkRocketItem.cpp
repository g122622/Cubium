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

#include "FireworkRocketItem.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ProjectileItem.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include <memory>

namespace mc {
namespace item {

FireworkRocketItem::FireworkRocketItem(const ItemProperties& properties)
    : Item(properties)
    , ProjectileItem()
{}

std::unique_ptr<entity::ProjectileEntity> FireworkRocketItem::asProjectile(IWorld& world,
    const Vector3& position,
    const ItemStack& stack,
    f32 /*directionX*/,
    f32 /*directionY*/,
    f32 /*directionZ*/) const
{
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = world.entityRegistry();
    if (registry == nullptr) {
        return nullptr;
    }
    auto entity = entity::FireworkRocketEntity::create(&world, *registry);
    if (entity) {
        // 必须在 setFireworkItem 之前 setWorld，以便 _ensureLifeTimeComputed 能访问世界 RNG
        entity->setWorld(&world);
        entity->setPosition(position.x, position.y, position.z);
        auto* firework = dynamic_cast<entity::FireworkRocketEntity*>(entity.get());
        if (firework && !stack.isEmpty()) {
            // 设置烟花物品数据（飞行时间、爆炸效果等）
            firework->setFireworkItem(stack);
        }
        entity->setTypeId(entity::EntityTypeKeys::FIREWORK_ROCKET);
    }
    // FireworkRocketEntity 继承自 ProjectileEntity，安全的 unique_ptr 转换
    return std::unique_ptr<entity::ProjectileEntity>(static_cast<entity::ProjectileEntity*>(entity.release()));
}

ProjectileDispenseConfig FireworkRocketItem::getDispenseConfig() const
{
    return ProjectileDispenseConfig::fireworkRocket();
}

} // namespace item
} // namespace mc
