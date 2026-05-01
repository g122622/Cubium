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
    ItemStack heldStack = player.getHeldItem(hand);

    // 创建投掷实体
    entity::ProjectileItemEntity* projectile = createProjectile(world, player, heldStack);
    if (projectile == nullptr) {
        // TODO: 实体创建失败，可能是因为实体类未完全实现
        // 暂时只消耗物品
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

    // 生成实体
    world.spawnEntity(std::unique_ptr<Entity>(projectile));

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
    // TODO: 播放投掷音效
    // player.playSound(SoundEvents.ENTITY_SNOWBALL_THROW, 0.5F, 0.4F / (random.nextFloat() * 0.4F + 0.8F));
}

} // namespace item
} // namespace mc
