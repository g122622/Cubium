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
 */

#include "FlowerBedBlock.hpp"

#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"
#include "common/world/block/core/Block.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

FlowerBedBlock::FlowerBedBlock(const BlockProperties& properties)
    : BushBlock(properties)
{
    // 创建状态容器：FACING + AMOUNT 两个属性
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(FACING()).add(AMOUNT()).create(
        [](const Block& block,
            StateValueIndices valueIndices,
            size_t propertyCount,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) {
            return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
        });
    createBlockState(std::move(container));

    // 默认状态：朝北、1个花瓣
    setDefaultState(defaultState().with(FACING(), Direction::North).with(AMOUNT(), 1));
}

// ========== 放置逻辑 ==========

BlockState FlowerBedBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 检查目标位置是否已有同类型花瓣床（堆叠逻辑）
    const BlockState* existingState = world.getBlockState(pos);
    if (existingState != nullptr && existingState->is(this)) {
        i32 currentAmount = existingState->get(AMOUNT());
        if (currentAmount < 4) {
            // 增加花瓣数量，保持原朝向
            return existingState->with(AMOUNT(), currentAmount + 1);
        }
        // 已满4个，无法继续堆叠
        return *existingState;
    }

    // 新放置：朝向为玩家水平朝向的反方向
    Direction facing = context.horizontalDirection();
    Direction oppositeFacing = Directions::opposite(facing);
    return defaultState().with(FACING(), oppositeFacing).with(AMOUNT(), 1);
}

bool FlowerBedBlock::isReplaceable(const BlockState& state, const BlockItemUseContext& context) const
{
    // 条件1：玩家未潜行
    Player* player = context.getPlayer();
    if (player != nullptr && player->isSneaking()) {
        // 潜行时不堆叠，但仍然可被其他方块替换（回退到基类行为）
        return BushBlock::isReplaceable(state, context);
    }

    // 条件2：手持物品必须是此方块对应的物品
    const ItemStack& heldItem = context.getItemStack();
    if (heldItem.isEmpty()) {
        // 空手不能堆叠，但仍然可被其他方块替换
        return BushBlock::isReplaceable(state, context);
    }

    // 条件3：通过 BlockItemRegistry 检查手持物品是否为此方块的 BlockItem
    const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(*this);
    if (blockItem == nullptr || heldItem.getItem() != blockItem) {
        // 手持非同类型物品不能堆叠，但仍然可被其他方块替换
        return BushBlock::isReplaceable(state, context);
    }

    // 条件4：当前 AMOUNT < 4 时允许堆叠替换
    return state.get(AMOUNT()) < 4;
}

// ========== 旋转/镜像 ==========

const BlockState& FlowerBedBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction currentFacing = _getFacing(state);
    Direction newFacing = currentFacing;

    switch (rotation) {
        case Rotation::Clockwise90:
            newFacing = Directions::rotateY(currentFacing);
            break;
        case Rotation::Clockwise180:
            newFacing = Directions::opposite(currentFacing);
            break;
        case Rotation::CounterClockwise90:
            newFacing = Directions::rotateYCCW(currentFacing);
            break;
        case Rotation::None:
        default:
            break;
    }

    return state.with(FACING(), newFacing);
}

const BlockState& FlowerBedBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction currentFacing = _getFacing(state);
    Direction newFacing = currentFacing;

    switch (mirror) {
        case Mirror::LeftRight:
            if (currentFacing == Direction::East) {
                newFacing = Direction::West;
            } else if (currentFacing == Direction::West) {
                newFacing = Direction::East;
            }
            break;
        case Mirror::FrontBack:
            if (currentFacing == Direction::North) {
                newFacing = Direction::South;
            } else if (currentFacing == Direction::South) {
                newFacing = Direction::North;
            }
            break;
        case Mirror::None:
        default:
            break;
    }

    return state.with(FACING(), newFacing);
}

// ========== 骨粉 ==========

bool FlowerBedBlock::canGrow(IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(isClientSide);
    return true;
}

bool FlowerBedBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    return true;
}

void FlowerBedBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    i32 amount = state.get(AMOUNT());
    if (amount < 4) {
        // 增加1个花瓣
        world.setBlockState(pos, &state.with(AMOUNT(), amount + 1), world::BlockUpdateFlags::UPDATE_CLIENTS);
    } else {
        // 已满4个，弹出一个物品
        const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(*this);
        if (blockItem != nullptr) {
            ItemStack dropStack(blockItem, 1);
            // 使用方块掉落速度生成物品实体
            math::Random rng(random.nextLong());
            Vector3 velocity = ItemDropHelper::getBlockDropVelocity(rng);
            ItemDropHelper::spawnItemEntity(&world,
                dropStack,
                pos.x + 0.5,
                pos.y + 0.5,
                pos.z + 0.5,
                static_cast<f32>(velocity.x),
                static_cast<f32>(velocity.y),
                static_cast<f32>(velocity.z),
                ItemDropHelper::DEFAULT_PICKUP_DELAY);
        }
    }
}

// ========== 形状 ==========

const CollisionShape& FlowerBedBlock::getShape(const BlockState& state) const
{
    // 原版 `pink_petals` / `wildflowers` 的 getShape 与 leaf_litter 同构：
    // 从 FACING 指向的那个角开始铺开。逐条实测（forAllBoxes）：
    //
    //   north n1 -> [0,0,0 -> .5,.19,.5]    n2 -> [0,0,0 -> .5,.19,1]
    //   east  n1 -> [.5,0,0 -> 1,.19,.5]    n2 -> [0,0,0 -> 1,.19,.5]
    //   south n1 -> [.5,0,.5 -> 1,.19,1]    n2 -> [.5,0,0 -> 1,.19,1]
    //   west  n1 -> [0,0,.5 -> .5,.19,1]    n2 -> [0,0,.5 -> 1,.19,1]
    //
    // n3 / n4 一律满格。此前用的是四象限映射，与「从一角铺开」的语义无关。
    const i32 amount = _getAmount(state);
    const Direction facing = _getFacing(state);
    MC_ASSERT(amount >= 1 && amount <= 4);

    struct Entry {
        Direction facing;
        i32 amount;
        CollisionShape shape;
    };

    static const std::array<Entry, 8> entries = {{
        {Direction::North, 1, CollisionShape::box(0.0f, 0.0f, 0.0f, 0.5f, 0.1875f, 0.5f)},
        {Direction::North, 2, CollisionShape::box(0.0f, 0.0f, 0.0f, 0.5f, 0.1875f, 1.0f)},
        {Direction::East, 1, CollisionShape::box(0.5f, 0.0f, 0.0f, 1.0f, 0.1875f, 0.5f)},
        {Direction::East, 2, CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.1875f, 0.5f)},
        {Direction::South, 1, CollisionShape::box(0.5f, 0.0f, 0.5f, 1.0f, 0.1875f, 1.0f)},
        {Direction::South, 2, CollisionShape::box(0.5f, 0.0f, 0.0f, 1.0f, 0.1875f, 1.0f)},
        {Direction::West, 1, CollisionShape::box(0.0f, 0.0f, 0.5f, 0.5f, 0.1875f, 1.0f)},
        {Direction::West, 2, CollisionShape::box(0.0f, 0.0f, 0.5f, 1.0f, 0.1875f, 1.0f)},
    }};
    static const CollisionShape fullShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.1875f, 1.0f);

    if (amount >= 3) {
        return fullShape;
    }
    for (const Entry& entry : entries) {
        if (entry.facing == facing && entry.amount == amount) {
            return entry.shape;
        }
    }
    MC_ASSERT_RELEASE_MSG(false, "flower bed: no shape for the given facing/amount");
    return fullShape;
}

i32 FlowerBedBlock::_getAmount(const BlockState& state) const
{
    return state.get(AMOUNT());
}

Direction FlowerBedBlock::_getFacing(const BlockState& state) const
{
    return state.get(FACING());
}

} // namespace blocks
} // namespace mc
