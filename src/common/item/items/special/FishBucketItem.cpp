#include "FishBucketItem.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/BlockState.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/core/EntityRegistry.hpp"
#include "../../../entity/core/EntityType.hpp"
#include "../../../item/context/BlockItemUseContext.hpp"
#include "../../../player/Player.hpp"
#include "../../../item/core/ItemStack.hpp"

namespace mc {
namespace item {

FishBucketItem::FishBucketItem(
    entity::EntityType fishType,
    const ItemProperties& properties)
    : Item(properties)
    , m_fishType(fishType) {
}

ActionResultType FishBucketItem::onItemUse(ItemUseContext& context) {
    IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();
    Direction face = context.getFace();

    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    // 计算放置位置
    BlockPos placePos = pos.offset(face);

    // 检查是否可以放置水
    const BlockState* currentState = world.getBlockState(placePos);
    if (currentState == nullptr) {
        return ActionResultType::Fail;
    }

    // 放置水方块
    const BlockState* waterState = VanillaBlocks::getState(VanillaBlocks::WATER);
    if (waterState != nullptr) {
        world.setBlockState(placePos, waterState, 3);
    }

    // 在水中生成鱼
    if (spawnFish(world, placePos)) {
        // 返回空桶 (非创造模式)
        if (!context.getPlayer().isCreative()) {
            // TODO: 返回空桶
            context.getItemStack().shrink(1);
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Fail;
}

ItemActionResult FishBucketItem::onItemRightClick(IWorld& world, Player& player, Hand hand) {
    if (world.isClientSide()) {
        return ItemActionResult::success(player.getHeldItem(hand));
    }

    // 检查玩家是否在水中
    if (player.isInWater()) {
        BlockPos spawnPos(
            static_cast<i32>(player.getX()),
            static_cast<i32>(player.getY()),
            static_cast<i32>(player.getZ())
        );

        if (spawnFish(world, spawnPos)) {
            if (!player.isCreative()) {
                // TODO: 返回空桶
                player.getHeldItem(hand).shrink(1);
            }
            return ItemActionResult::success(player.getHeldItem(hand));
        }
    }

    return ItemActionResult::pass(player.getHeldItem(hand));
}

bool FishBucketItem::spawnFish(IWorld& world, const BlockPos& pos) const {
    // 创建鱼实体
    auto fish = entity::EntityRegistry::instance().createEntity(m_fishType, world);
    if (!fish) {
        return false;
    }

    // 设置位置 (方块中心)
    f32 x = static_cast<f32>(pos.x) + 0.5f;
    f32 y = static_cast<f32>(pos.y) + 0.5f;
    f32 z = static_cast<f32>(pos.z) + 0.5f;
    fish->setPosition(x, y, z);

    // TODO: 设置鱼的 FromBucket 标签，防止消失

    // 生成实体
    world.spawnEntity(std::move(fish));
    return true;
}

} // namespace item
} // namespace mc
