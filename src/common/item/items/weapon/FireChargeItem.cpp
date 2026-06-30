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
 * IMPLIED, INCLUDING ANY PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "FireChargeItem.hpp"
#include "common/entity/entities/projectile/AbstractFireballEntity.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace item {

FireChargeItem::FireChargeItem(const ItemProperties& properties)
    : Item(properties)
    , ProjectileItem()
{}

std::unique_ptr<entity::ProjectileEntity> FireChargeItem::asProjectile(IWorld& world,
    const Vector3& position,
    const ItemStack& /*stack*/,
    f32 directionX,
    f32 directionY,
    f32 directionZ) const
{
    auto entity = entity::SmallFireballEntity::create(&world);
    if (entity) {
        entity->setPosition(position.x, position.y, position.z);
        // 火焰弹使用加速度驱动（而非速度），设置加速度方向
        auto* fireball = dynamic_cast<entity::SmallFireballEntity*>(entity.get());
        if (fireball) {
            // 加速度 = 方向 * 力度
            auto config = getDispenseConfig();
            fireball->setAcceleration(directionX * config.power, directionY * config.power, directionZ * config.power);
        }
    }
    // SmallFireballEntity 继承自 ProjectileEntity，安全的 unique_ptr 转换
    return std::unique_ptr<entity::ProjectileEntity>(static_cast<entity::ProjectileEntity*>(entity.release()));
}

ProjectileDispenseConfig FireChargeItem::getDispenseConfig() const
{
    return ProjectileDispenseConfig::fireCharge();
}

void FireChargeItem::shoot(entity::ProjectileEntity& /*projectile*/,
    f32 /*directionX*/,
    f32 /*directionY*/,
    f32 /*directionZ*/,
    f32 /*power*/,
    f32 /*uncertainty*/) const
{
    // 火焰弹在 asProjectile() 中已设置加速度，不需要 shoot()
}

} // namespace item
} // namespace mc
