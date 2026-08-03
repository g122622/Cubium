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

#include "TippedArrowItem.hpp"
#include "../../../entity/core/EntityRegistry.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/entities/projectile/AbstractArrowEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../potion/PotionUtils.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/weapon/ArrowItem.hpp"
#include "common/item/potion/Potion.hpp"
#include "common/util/math/Vector3.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace item {

// NBT 键
static constexpr const char* NBT_POTION = "Potion";
static constexpr const char* NBT_CUSTOM_POTION_EFFECTS = "CustomPotionEffects";

TippedArrowItem::TippedArrowItem(const ItemProperties& properties)
    : ArrowItem(properties)
{}

entity::AbstractArrowEntity* TippedArrowItem::createArrow(
    IWorld& world, const ItemStack& stack, LivingEntity& shooter) const
{
    // 创建箭矢实体
    auto arrow = entity::ArrowEntity::createFromShooter(shooter, &world);
    if (!arrow) {
        return nullptr;
    }

    // 获取药水效果
    auto effects = getEffects(stack);
    if (!effects.empty()) {
        // 设置药水效果到箭矢
        arrow->setEffects(effects);

        // 设置箭矢颜色
        u32 color = potion::PotionUtils::getColor(effects);
        arrow->setColor(color);
    }

    return arrow.release();
}

bool TippedArrowItem::isInfinite(const ItemStack& /*arrowStack*/, const ItemStack& /*bowStack*/, Player& player) const
{
    // MC 1.16.5: 药水箭不受益于无限附魔
    // 只有创造模式下才能无限使用
    return player.isCreative();
}

std::unique_ptr<entity::ProjectileEntity> TippedArrowItem::asProjectile(IWorld& world,
    const Vector3& position,
    const ItemStack& stack,
    f32 /*directionX*/,
    f32 /*directionY*/,
    f32 /*directionZ*/) const
{
    auto entity = entity::ArrowEntity::create(&world);
    if (entity) {
        entity->setTypeId(entity::EntityTypeKeys::ARROW);
        entity->setPosition(position.x, position.y, position.z);
        auto* arrow = dynamic_cast<entity::ArrowEntity*>(entity.get());
        if (arrow) {
            arrow->setPickupStatus(entity::PickupStatus::Allowed);
            // 从 ItemStack 读取药水效果并应用到箭矢
            auto effects = getEffects(stack);
            if (!effects.empty()) {
                arrow->setEffects(effects);
                arrow->setColor(potion::PotionUtils::getColor(effects));
            }
        }
    }
    // ArrowEntity 继承自 ProjectileEntity，安全的 unique_ptr 转换
    return std::unique_ptr<entity::ProjectileEntity>(static_cast<entity::ProjectileEntity*>(entity.release()));
}

const potion::Potion* TippedArrowItem::getPotion(const ItemStack& stack)
{
    // 使用 PotionUtils 获取药水
    return potion::PotionUtils::getPotion(stack);
}

std::vector<entity::effect::EffectInstance> TippedArrowItem::getEffects(const ItemStack& stack)
{
    // 获取药水的基础效果
    std::vector<entity::effect::EffectInstance> effects;

    const potion::Potion* potion = getPotion(stack);
    if (potion != nullptr) {
        effects = potion::PotionUtils::getEffects(potion);
    }

    // 注意：自定义效果（NBT CustomPotionEffects）需要在 NBT 系统完善后合并
    // 当前仅返回药水的基础效果

    return effects;
}

void TippedArrowItem::setPotion(ItemStack& stack, const potion::Potion* potion)
{
    potion::PotionUtils::setPotion(stack, potion);
}

} // namespace item
} // namespace mc
