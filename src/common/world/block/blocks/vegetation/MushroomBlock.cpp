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

#include "MushroomBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../IWorld.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/PlantType.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

namespace {

[[nodiscard]] bool canSustainMushroom(const BlockState& groundState, const IWorld& world, const BlockPos& mushroomPos)
{
    // 如果下方方块属于 MUSHROOM_GROW_BLOCK 标签（菌丝、灰化土、绯红菌岩、诡异菌岩），
    // 则无条件允许放置（不受光照限制）
    if (BlockTags::MUSHROOM_GROW_BLOCK().contains(groundState)) {
        return true;
    }

    // 其他方块：必须是固体，且光照 < 13
    if (!groundState.isSolid()) {
        return false;
    }

    const i32 blockLight = static_cast<i32>(world.getBlockLight(mushroomPos));
    const i32 skyLight = static_cast<i32>(world.getSkyLight(mushroomPos));
    return std::max(blockLight, skyLight) < 13;
}

} // namespace

// ========== MushroomBlock ==========

MushroomBlock::MushroomBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 蘑菇形状：小型圆形
    m_shape = CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 0.5f, 0.75f);
}

BlockState MushroomBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState();
}

bool MushroomBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    return canSustainMushroom(*belowState, world, pos);
}

void MushroomBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(state);

    const i32 blockLight = static_cast<i32>(world.getBlockLight(pos));
    const i32 skyLight = static_cast<i32>(world.getSkyLight(pos));
    if (std::max(blockLight, skyLight) >= 13) {
        return;
    }

    if (random.nextInt(25) != 0) {
        return;
    }

    i32 nearbyMushrooms = 0;
    for (i32 dx = -4; dx <= 4; ++dx) {
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dz = -4; dz <= 4; ++dz) {
                const BlockState* nearbyState = world.getBlockState(pos.x + dx, pos.y + dy, pos.z + dz);
                if (nearbyState != nullptr && nearbyState->is(this)) {
                    ++nearbyMushrooms;
                    if (nearbyMushrooms >= 5) {
                        return;
                    }
                }
            }
        }
    }

    const BlockPos spreadPos(
        pos.x + random.nextInt(3) - 1, pos.y + random.nextInt(2) - 1, pos.z + random.nextInt(3) - 1);

    const BlockState* targetState = world.getBlockState(spreadPos);
    if (targetState != nullptr && !targetState->isAir()) {
        return;
    }

    const BlockPos belowTarget = spreadPos.down();
    const BlockState* belowState = world.getBlockState(belowTarget);
    if (belowState == nullptr) {
        return;
    }

    if (!canSustainMushroom(*belowState, world, spreadPos)) {
        return;
    }

    const BlockState& mushroomState = defaultState();
    world.setBlockState(spreadPos, &mushroomState, 2);
}

const CollisionShape& MushroomBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& MushroomBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 蘑菇没有碰撞箱
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== IPlantable 接口实现 ==========

PlantType MushroomBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return PlantType::Cave;
}

const BlockState& MushroomBlock::getPlant(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    return defaultState();
}

// ========== HugeMushroomBlock ==========

HugeMushroomBlock::HugeMushroomBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器（6个方向的布尔属性）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::DOWN())
            .add(BlockStateProperties::UP())
            .add(BlockStateProperties::NORTH())
            .add(BlockStateProperties::SOUTH())
            .add(BlockStateProperties::EAST())
            .add(BlockStateProperties::WEST())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态：所有面都显示蘑菇皮纹理
    setDefaultState(defaultState()
            .with(BlockStateProperties::DOWN(), true)
            .with(BlockStateProperties::UP(), true)
            .with(BlockStateProperties::NORTH(), true)
            .with(BlockStateProperties::SOUTH(), true)
            .with(BlockStateProperties::EAST(), true)
            .with(BlockStateProperties::WEST(), true));
}

const BlockState& HugeMushroomBlock::rotate(const BlockState& state, Rotation rotation) const
{
    // 旋转各方向的面
    bool north = state.get(BlockStateProperties::NORTH());
    bool south = state.get(BlockStateProperties::SOUTH());
    bool east = state.get(BlockStateProperties::EAST());
    bool west = state.get(BlockStateProperties::WEST());

    switch (rotation) {
        case Rotation::None:
            return state;
        case Rotation::Clockwise90:
            return state.with(BlockStateProperties::NORTH(), west)
                .with(BlockStateProperties::SOUTH(), east)
                .with(BlockStateProperties::EAST(), north)
                .with(BlockStateProperties::WEST(), south);
        case Rotation::Clockwise180:
            return state.with(BlockStateProperties::NORTH(), south)
                .with(BlockStateProperties::SOUTH(), north)
                .with(BlockStateProperties::EAST(), west)
                .with(BlockStateProperties::WEST(), east);
        case Rotation::CounterClockwise90:
            return state.with(BlockStateProperties::NORTH(), east)
                .with(BlockStateProperties::SOUTH(), west)
                .with(BlockStateProperties::EAST(), south)
                .with(BlockStateProperties::WEST(), north);
        default:
            return state;
    }
}

const BlockState& HugeMushroomBlock::mirror(const BlockState& state, Mirror mirror) const
{
    switch (mirror) {
        case Mirror::None:
            return state;
        case Mirror::LeftRight: {
            bool north = state.get(BlockStateProperties::NORTH());
            bool south = state.get(BlockStateProperties::SOUTH());
            return state.with(BlockStateProperties::NORTH(), south).with(BlockStateProperties::SOUTH(), north);
        }
        case Mirror::FrontBack: {
            bool east = state.get(BlockStateProperties::EAST());
            bool west = state.get(BlockStateProperties::WEST());
            return state.with(BlockStateProperties::EAST(), west).with(BlockStateProperties::WEST(), east);
        }
        default:
            return state;
    }
}

const CollisionShape& HugeMushroomBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape fullShape = CollisionShape::fullBlock();
    return fullShape;
}

} // namespace blocks
} // namespace mc
