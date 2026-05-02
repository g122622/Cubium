#include "ThrowableItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../core/ActionResult.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/entities/projectile/ProjectileItemEntity.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../world/IWorld.hpp"
#include <cmath>

namespace mc {
namespace item {

// ========== 构造函数 ==========

ThrowableItem::ThrowableItem(const ItemProperties& properties)
    : Item(properties)
{
}

// ========== Item 接口重写 ==========

i32 ThrowableItem::getUseDuration(const ItemStack& /*stack*/) const {
    return 0;
}

ItemActionResult ThrowableItem::onItemRightClick(IWorld& world, Player& player, Hand hand) {
    ItemStack& heldStack = player.getHeldItem(hand);

    // 创建投掷实体
    entity::ProjectileItemEntity* projectile = createProjectile(world, player, heldStack);
    if (projectile == nullptr) {
        // 实体创建失败，只消耗物品
        if (!player.isCreative()) {
            heldStack.shrink(1);
        }
        return ItemActionResult::success(heldStack);
    }

    // 设置投掷参数
    // 参考 MC 1.16.5 ProjectileItemEntity.shoot()
    projectile->shootFrom(
        player,
        player.pitch(),
        player.yaw(),
        0.0f,                    // 滚动角
        getThrowVelocity(),
        getThrowInaccuracy()
    );

    // 播放投掷音效
    playThrowSound(player);

    // 消耗物品（非创造模式）
    if (!player.isCreative()) {
        heldStack.shrink(1);
    }

    return ItemActionResult::success(heldStack);
}

// ========== 投掷物品特有方法 ==========

void ThrowableItem::playThrowSound(Player& /*player*/) const {
    // 子类可覆盖以播放特定音效
}

} // namespace item
} // namespace mc
