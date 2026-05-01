#include "FishingRodItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../core/ActionResult.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/entities/projectile/OtherProjectiles.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../util/math/MathUtils.hpp"
#include <cmath>

namespace mc {
namespace item {

// ========== 构造函数 ==========

FishingRodItem::FishingRodItem(const ItemProperties& properties)
    : Item(properties)
{
}

// ========== Item 接口重写 ==========

i32 FishingRodItem::getUseDuration(const ItemStack& /*stack*/) const {
    return 0;
}

UseAction FishingRodItem::getUseAction(const ItemStack& /*stack*/) const {
    return UseAction::Bow;
}

ItemActionResult FishingRodItem::onItemRightClick(IWorld& world, Player& player, Hand hand) {
    ItemStack rodStack = player.getHeldItem(hand);

    // 检查玩家是否已经有浮标
    if (hasBobber(player)) {
        // 收杆
        entity::FishingBobberEntity* bobber = getBobber(player);
        if (bobber != nullptr) {
            i32 damage = bobber->reelIn();
            rodStack.attemptDamageItem(damage);
        }
        // TODO: 清除玩家的浮标引用（需要在 Player 中添加 fishingBobber 字段）
    } else {
        // 抛杆
        // TODO: 获取钓鱼附魔
        // i32 luckBonus = EnchantmentHelper::getFishingLuckBonus(rodStack);
        // i32 speedBonus = EnchantmentHelper::getFishingSpeedBonus(rodStack);
        (void)world;

        // TODO: 创建浮标实体（需要 FishingBobberEntity 完整实现和 Player.fishingBobber 字段）
        // auto bobber = std::make_unique<entity::FishingBobberEntity>(player, &world, luckBonus, speedBonus);
        // world.spawnEntity(std::move(bobber));
    }

    return ItemActionResult::success(rodStack);
}

// ========== 钓鱼竿特有方法 ==========

bool FishingRodItem::hasBobber(Player& player) {
    // TODO: 需要在 Player 中添加 fishingBobber 字段
    (void)player;
    return false;
}

entity::FishingBobberEntity* FishingRodItem::getBobber(Player& player) {
    // TODO: 需要在 Player 中添加 fishingBobber 字段
    (void)player;
    return nullptr;
}

} // namespace item
} // namespace mc
