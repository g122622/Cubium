#include "FishBucketItem.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../../world/fluid/Fluid.hpp"
#include "../../../world/fluid/FluidRegistry.hpp"
#include "../../../world/tick/manager/TickManager.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/core/EntityRegistry.hpp"
#include "../../../entity/core/EntityType.hpp"
#include "../../../entity/entities/passive/fish/AbstractFishEntity.hpp"
#include "../../../entity/utils/ItemDropHelper.hpp"
#include "../../context/BlockItemUseContext.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../core/ItemStack.hpp"
#include "../../Items.hpp"
#include "../../../util/math/random/Random.hpp"

namespace mc {
namespace item {

FishBucketItem::FishBucketItem(
    const char* fishTypeName,
    const ItemProperties& properties)
    : Item(properties)
    , m_fishTypeName(fishTypeName) {
}

ActionResultType FishBucketItem::onItemUse(ItemUseContext& context) {
    IWorld& world = context.getWorld();
    BlockPos pos = context.blockPos();
    Direction face = context.face();

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

        // 调度流体 tick
        fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
        if (waterFluid != nullptr) {
            world.tickManager().scheduleFluidTick(placePos, *waterFluid, waterFluid->getTickDelay(world));
        }
    }

    // 在水中生成鱼
    if (spawnFish(world, placePos)) {
        // 返回空桶（非创造模式）
        Player* player = context.getPlayer();
        if (player != nullptr && !player->isCreative()) {
            // 减少鱼桶数量
            context.getItemStackMut().shrink(1);
            // 返回空桶
            returnEmptyBucket(*player, context.getItemStackMut());
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
            static_cast<i32>(player.x()),
            static_cast<i32>(player.y()),
            static_cast<i32>(player.z())
        );

        if (spawnFish(world, spawnPos)) {
            if (!player.isCreative()) {
                player.getHeldItem(hand).shrink(1);
                // 返回空桶
                returnEmptyBucket(player, player.getHeldItem(hand));
            }
            return ItemActionResult::success(player.getHeldItem(hand));
        }
    }

    return ItemActionResult::pass(player.getHeldItem(hand));
}

bool FishBucketItem::spawnFish(IWorld& world, const BlockPos& pos) const {
    // 获取实体类型
    const entity::EntityType* fishType = entity::EntityRegistry::instance().getType(m_fishTypeName);
    if (fishType == nullptr) {
        return false;
    }

    // 创建鱼实体
    auto fish = fishType->create(&world);
    if (!fish) {
        return false;
    }

    // 设置位置 (方块中心)
    f32 x = static_cast<f32>(pos.x) + 0.5f;
    f32 y = static_cast<f32>(pos.y) + 0.5f;
    f32 z = static_cast<f32>(pos.z) + 0.5f;
    fish->setPosition(x, y, z);

    // 设置 FromBucket 标签，防止消失
    // 参考 MC 1.16.5 FishBucketItem.placeFish()
    auto* abstractFish = dynamic_cast<AbstractFishEntity*>(fish.get());
    if (abstractFish != nullptr) {
        abstractFish->setFromBucket(true);
    }

    // 生成实体
    world.spawnEntity(std::move(fish));
    return true;
}

void FishBucketItem::returnEmptyBucket(Player& player, ItemStack& stack) const {
    // 参考 MC 1.16.5 BucketItem.emptyBucket()
    // 如果物品堆已空，直接返回空桶
    if (stack.isEmpty() && Items::BUCKET != nullptr) {
        stack = ItemStack(Items::BUCKET, 1);
        return;
    }

    // 否则尝试将空桶添加到背包
    if (Items::BUCKET != nullptr) {
        ItemStack bucketStack(Items::BUCKET, 1);
        i32 remaining = player.inventory().add(bucketStack);

        // 如果背包满了，掉落到地面
        if (remaining > 0 && !bucketStack.isEmpty()) {
            // 使用 ItemDropHelper 掉落物品
            math::Random rng;
            ItemDropHelper::spawnItemAtEntity(&player, bucketStack, 0.5f, rng);
        }
    }
}

} // namespace item
} // namespace mc
