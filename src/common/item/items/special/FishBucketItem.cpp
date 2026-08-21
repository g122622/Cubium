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

#include "FishBucketItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/passive/fish/AbstractFishEntity.hpp"
#include "common/entity/entities/passive/water/AxolotlEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <utility>

namespace mc {
namespace item {

FishBucketItem::FishBucketItem(const char* fishTypeName, const ItemProperties& properties)
    : Item(properties)
    , m_fishTypeName(fishTypeName)
{}

ActionResultType FishBucketItem::onItemUse(ItemUseContext& context)
{
    IWorld& world = context.getWorld();
    BlockPos pos = context.blockPos();
    Direction face = context.face();
    Player* player = context.getPlayer();
    const Hand hand = context.getHand();

    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    // 计算放置位置
    BlockPos placePos = pos.offset(face);

    // 放置水方块
    const BlockState* waterState = VanillaBlocks::getState(VanillaBlocks::WATER);
    world.setBlockState(placePos, waterState, 3);

    // 调度流体 tick
    fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
    world.tickManager().scheduleFluidTick(placePos, *waterFluid, waterFluid->getTickDelay(world));

    // 在水中生成鱼
    if (_spawnFish(world, placePos)) {
        // 返回空桶（对齐 Java 1.21.11 MobBucketItem.use：消耗手持鱼桶并替换为空桶）。
        // 必须直接操作玩家权威手持物（player->getHeldItem(hand) 返回引用），而非 context 拷贝——
        // ItemUseContext 内的 stack 是调用方局部拷贝，对其修改不回写权威物品栏。
        // 此前实现用 context.getItemStackMut()（拷贝）做 shrink+_returnEmptyBucket，拷贝上的赋值
        // 无效，权威手持鱼桶未被替换为空桶——生产路径外层 handleItemUseOn 的通用 shrink(1) 仅把
        // 鱼桶变空堆，玩家失去鱼桶却得不到空桶（对齐缺陷）。改为直接写权威手持修复。
        if (player != nullptr && !player->isCreative()) {
            ItemStack& heldItem = player->getHeldItem(hand);
            // 先消耗一个鱼桶（鱼桶 maxStack=1，shrink(1) 后 count=0→isEmpty），再 _returnEmptyBucket
            // 把空堆替换为空桶（对齐 vanilla MobBucketItem.use：getItemInHand shrink 后 setItemInHand
            // 空桶）。_returnEmptyBucket 契约：调用方先 shrink 使 stack 空，其内 stack.isEmpty() 命中
            // 第一分支替换为空桶；未 shrink 则走第二分支加背包（非预期）。
            heldItem.shrink(1);
            _returnEmptyBucket(*player, heldItem);
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Fail;
}

ItemActionResult FishBucketItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    if (world.isClientSide()) {
        return ItemActionResult::success(player.getHeldItem(hand));
    }

    // 检查玩家是否在水中
    if (player.isInWater()) {
        BlockPos spawnPos(static_cast<i32>(player.x()), static_cast<i32>(player.y()), static_cast<i32>(player.z()));

        if (_spawnFish(world, spawnPos)) {
            if (!player.isCreative()) {
                // 同 onItemUse：直接操作权威手持物，先 shrink 再 _returnEmptyBucket 替换为空桶
                // （对齐 vanilla MobBucketItem.use）。
                ItemStack& heldItem = player.getHeldItem(hand);
                heldItem.shrink(1);
                _returnEmptyBucket(player, heldItem);
            }
            return ItemActionResult::success(player.getHeldItem(hand));
        }
    }

    return ItemActionResult::pass(player.getHeldItem(hand));
}

bool FishBucketItem::_spawnFish(IWorld& world, const BlockPos& pos) const
{
    // 获取实体类型
    const entity::EntityType* fishType = entity::EntityRegistry::instance().getType(m_fishTypeName);
    if (fishType == nullptr) {
        return false;
    }

    // 创建鱼实体
    // 通过世界获取 ECS 实体注册表（ServerWorld 持有 m_entityRegistry）
    auto* registry = world.entityRegistry();
    if (registry == nullptr) {
        return false;
    }
    auto fish = fishType->create(&world, *registry);
    if (!fish) {
        return false;
    }

    // 设置位置（方块中心）
    f32 x = static_cast<f32>(pos.x) + 0.5f;
    f32 y = static_cast<f32>(pos.y) + 0.5f;
    f32 z = static_cast<f32>(pos.z) + 0.5f;
    fish->setPosition(x, y, z);

    // 设置 FromBucket 标签，防止消失
    auto* abstractFish = dynamic_cast<AbstractFishEntity*>(fish.get());
    if (abstractFish != nullptr) {
        abstractFish->setFromBucket(true);
    }

    // 美西螈也有类似的 FromBucket 标记
    auto* axolotl = dynamic_cast<AxolotlEntity*>(fish.get());
    if (axolotl != nullptr) {
        axolotl->setFromBucket(true);
    }

    // 生成实体
    world.spawnEntity(std::move(fish));
    return true;
}

void FishBucketItem::_returnEmptyBucket(Player& player, ItemStack& stack) const
{
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
