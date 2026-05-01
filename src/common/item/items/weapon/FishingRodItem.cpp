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
    // 如果有，收杆；如果没有，抛杆
    // TODO: 需要在 Player 中存储 FishingBobberEntity 引用

    // TODO: 创建浮标实体 (FishingBobberEntity 未完全实现)
    // auto bobber = std::make_unique<entity::FishingBobberEntity>(LegacyEntityType::Unknown, 0);
    // ... 设置位置和速度 ...
    // world.spawnEntity(std::move(bobber));

    // 消耗耐久度
    rodStack.attemptDamageItem(1);

    return ItemActionResult::success(rodStack);
}

// ========== 钓鱼竿特有方法 ==========

bool FishingRodItem::hasBobber(Player& player) {
    // TODO: 检查玩家是否有浮标
    (void)player;
    return false;
}

entity::FishingBobberEntity* FishingRodItem::getBobber(Player& player) {
    // TODO: 返回玩家的浮标
    (void)player;
    return nullptr;
}

} // namespace item
} // namespace mc
