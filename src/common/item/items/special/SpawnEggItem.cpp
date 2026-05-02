#include "SpawnEggItem.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/core/EntityType.hpp"
#include "../../../entity/core/EntityRegistry.hpp"
#include "../../../item/context/BlockItemUseContext.hpp"
#include "../../../player/Player.hpp"
#include "../../../util/math/Vector3.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../../core/Direction.hpp"

namespace mc {
namespace item {

SpawnEggItem::SpawnEggItem(
    entity::EntityType entityType,
    u32 primaryColor,
    u32 secondaryColor,
    const ItemProperties& properties)
    : Item(properties)
    , m_entityType(entityType)
    , m_primaryColor(primaryColor)
    , m_secondaryColor(secondaryColor) {
}

ActionResultType SpawnEggItem::onItemUse(ItemUseContext& context) {
    IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();
    Direction face = context.getFace();

    // 检查是否在客户端
    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    // 计算生成位置 (方块面偏移)
    BlockPos spawnPos = pos.offset(face);

    // 检查位置是否有效
    const BlockState* state = world.getBlockState(spawnPos);
    if (state != nullptr && !state->isAir() && !state->getMaterial().isReplaceable()) {
        return ActionResultType::Fail;
    }

    // 生成实体
    if (spawnEntity(world, spawnPos, entity::SpawnReason::SpawnEgg)) {
        // 消耗物品 (非创造模式)
        if (!context.getPlayer().isCreative()) {
            context.getItemStack().shrink(1);
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Fail;
}

ItemActionResult SpawnEggItem::onItemRightClick(IWorld& world, Player& player, Hand hand) {
    // 在玩家位置生成实体
    if (world.isClientSide()) {
        return ItemActionResult::success(player.getHeldItem(hand));
    }

    BlockPos spawnPos(
        static_cast<i32>(player.getX()),
        static_cast<i32>(player.getY()),
        static_cast<i32>(player.getZ())
    );

    if (spawnEntity(world, spawnPos, entity::SpawnReason::SpawnEgg)) {
        if (!player.isCreative()) {
            player.getHeldItem(hand).shrink(1);
        }
        return ItemActionResult::success(player.getHeldItem(hand));
    }

    return ItemActionResult::pass(player.getHeldItem(hand));
}

bool SpawnEggItem::spawnEntity(IWorld& world, const BlockPos& pos, entity::SpawnReason spawnReason) const {
    // 通过实体注册表创建实体
    auto entity = entity::EntityRegistry::instance().createEntity(m_entityType, world);
    if (!entity) {
        return false;
    }

    // 设置位置 (方块中心上方)
    f32 x = static_cast<f32>(pos.x) + 0.5f;
    f32 y = static_cast<f32>(pos.y);
    f32 z = static_cast<f32>(pos.z) + 0.5f;
    entity->setPosition(x, y, z);

    // 生成实体
    world.spawnEntity(std::move(entity));
    return true;
}

} // namespace item
} // namespace mc
