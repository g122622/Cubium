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

#include "LightningRodBlock.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/DirectionalBlock.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "util/property/Properties.hpp"
#include "world/block/WaterLoggableHelpers.hpp"
#include "world/redstone/RedstoneSystem.hpp"
#include "world/tick/manager/TickManager.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// 避雷针杆的形状（像素坐标）:
// 中心杆: 6x0x6 到 10x16x10（向上指时）
// 底座: 4x0x4 到 12x2x12
static CollisionShape makeRodShape(Direction facing)
{
    constexpr float rodMin = 6.0f / 16.0f;
    constexpr float rodMax = 10.0f / 16.0f;
    constexpr float baseMin = 4.0f / 16.0f;
    constexpr float baseMax = 12.0f / 16.0f;
    constexpr float baseH = 2.0f / 16.0f;
    constexpr float full = 1.0f;

    CollisionShape rod;
    CollisionShape base;

    switch (facing) {
        case Direction::Up:
            rod = CollisionShape::box(rodMin, 0.0f, rodMin, rodMax, full, rodMax);
            base = CollisionShape::box(baseMin, 0.0f, baseMin, baseMax, baseH, baseMax);
            break;
        case Direction::Down:
            rod = CollisionShape::box(rodMin, 0.0f, rodMin, rodMax, full, rodMax);
            base = CollisionShape::box(baseMin, full - baseH, baseMin, baseMax, full, baseMax);
            break;
        case Direction::North:
            rod = CollisionShape::box(rodMin, rodMin, 0.0f, rodMax, rodMax, full);
            base = CollisionShape::box(baseMin, baseMin, full - baseH, baseMax, baseMax, full);
            break;
        case Direction::South:
            rod = CollisionShape::box(rodMin, rodMin, 0.0f, rodMax, rodMax, full);
            base = CollisionShape::box(baseMin, baseMin, 0.0f, baseMax, baseMax, baseH);
            break;
        case Direction::East:
            rod = CollisionShape::box(0.0f, rodMin, rodMin, full, rodMax, rodMax);
            base = CollisionShape::box(0.0f, baseMin, baseMin, baseH, baseMax, baseMax);
            break;
        case Direction::West:
            rod = CollisionShape::box(0.0f, rodMin, rodMin, full, rodMax, rodMax);
            base = CollisionShape::box(full - baseH, baseMin, baseMin, full, baseMax, baseMax);
            break;
        default:
            rod = CollisionShape::box(rodMin, 0.0f, rodMin, rodMax, full, rodMax);
            base = CollisionShape::box(baseMin, 0.0f, baseMin, baseMax, baseH, baseMax);
            break;
    }

    return CollisionShape::combine(rod, base);
}

LightningRodBlock::LightningRodBlock(const BlockProperties& properties)
    : DirectionalBlock(properties)
{
    // DirectionalBlock 构造函数中已通过 Builder 添加了 FACING，
    // 但我们需要额外添加 POWERED 和 WATERLOGGED 属性，因此重建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(FACING())
            .add(BlockStateProperties::POWERED())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(FACING(), Direction::North)
            .with(BlockStateProperties::POWERED(), false)
            .with(BlockStateProperties::WATERLOGGED(), false));

    // 预计算6个方向的形状（充能状态不影响形状）
    for (int i = 0; i < 6; ++i) {
        Direction dir = static_cast<Direction>(i);
        m_shapes[i] = makeRodShape(dir);
        m_shapes[i + 6] = m_shapes[i]; // powered状态使用相同形状
    }
}

void LightningRodBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 属性已在构造函数中通过 Builder 添加
    MC_UNUSED(container);
}

BlockState LightningRodBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 避雷针朝向玩家视线方向的相反方向
    auto directions = context.getNearestLookingDirections();
    Direction facing = directions.empty() ? Direction::Up : directions.front();

    BlockState state = defaultState().with(FACING(), facing).with(BlockStateProperties::POWERED(), false);

    // 处理含水
    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    } else {
        state = state.with(BlockStateProperties::WATERLOGGED(), false);
    }

    return state;
}

BlockState LightningRodBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // 处理含水方块的水tick
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

void LightningRodBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 检查红石信号变化
    bool isPowered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, pos);
    bool wasPowered = world.getBlockState(pos)->get(BlockStateProperties::POWERED());

    if (isPowered != wasPowered) {
        world.setBlockState(pos, &world.getBlockState(pos)->with(BlockStateProperties::POWERED(), isPowered), 3);

        if (isPowered) {
            // 激活时安排tick来关闭
            world.tickManager().scheduleBlockTick(pos, *this, ACTIVATION_TICKS);
        }
    }
}

void LightningRodBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // 充能到期后关闭
    if (state.get(BlockStateProperties::POWERED())) {
        bool stillPowered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, pos);
        if (!stillPowered) {
            world.setBlockState(pos, &state.with(BlockStateProperties::POWERED(), false), 3);
        } else {
            // 仍然被充能，继续安排tick
            world.tickManager().scheduleBlockTick(pos, *this, ACTIVATION_TICKS);
        }
    }
}

i32 LightningRodBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    if (!state.get(BlockStateProperties::POWERED())) {
        return 0;
    }

    // 只在指向方向输出信号
    Direction facing = state.get(FACING());
    if (side == facing) {
        return 15;
    }
    return 0;
}

i32 LightningRodBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);
    // 避雷针不提供强信号
    return 0;
}

const CollisionShape& LightningRodBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(FACING());
    bool powered = state.get(BlockStateProperties::POWERED());
    size_t idx = _getShapeIndex(facing, powered);
    return m_shapes[idx];
}

const BlockState& LightningRodBlock::rotate(const BlockState& state, Rotation rotation) const
{
    return DirectionalBlock::rotate(state, rotation);
}

const BlockState& LightningRodBlock::mirror(const BlockState& state, Mirror mirror) const
{
    return DirectionalBlock::mirror(state, mirror);
}

const fluid::FluidState* LightningRodBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

void LightningRodBlock::onLightningStrike(IWorld& world, const BlockPos& pos)
{
    const BlockState* currentState = world.getBlockState(pos);
    if (currentState == nullptr || !currentState->is(this)) {
        return;
    }

    BlockState newState = currentState->with(BlockStateProperties::POWERED(), true);
    world.setBlockState(pos, &newState, 3);
    world.tickManager().scheduleBlockTick(pos, *this, ACTIVATION_TICKS);
}

void LightningRodBlock::handlePrecipitation(
    IWorld& world, const BlockPos& pos, world::biome::BiomeClimate::Precipitation precipitation)
{
    // 避雷针仅在雷暴天气且朝上时响应降水
    // 参考: net.minecraft.block.LightningRodBlock#handlePrecipitation
    if (precipitation != world::biome::BiomeClimate::Precipitation::Rain) {
        return;
    }

    // 必须正在雷暴
    if (!world.isThundering()) {
        return;
    }

    // 避雷针必须朝上才能被雷击
    const BlockState* currentState = world.getBlockState(pos);
    if (currentState == nullptr || !currentState->is(this)) {
        return;
    }

    if (currentState->get(FACING()) != Direction::Up) {
        return;
    }

    // 雷暴时朝上的避雷针激活
    onLightningStrike(world, pos);
}

size_t LightningRodBlock::_getShapeIndex(Direction facing, bool powered) noexcept
{
    return static_cast<size_t>(facing) + (powered ? 6 : 0);
}

} // namespace blocks
} // namespace mc
