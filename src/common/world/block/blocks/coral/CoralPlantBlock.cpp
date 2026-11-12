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

#include "common/world/block/blocks/coral/CoralPlantBlock.hpp"

#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/capability/WaterLoggableHelpers.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/support/SupportType.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "util/math/random/IRandom.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

namespace {

[[nodiscard]] bool hasNearbyWater(mc::IWorld& world, const mc::BlockPos& pos)
{
    for (mc::Direction dir : {mc::Direction::North,
             mc::Direction::South,
             mc::Direction::East,
             mc::Direction::West,
             mc::Direction::Up,
             mc::Direction::Down}) {
        const mc::fluid::FluidState* fluidState = world.getFluidState(pos.offset(dir));
        if (mc::waterloggable::isWaterFluidState(fluidState)) {
            return true;
        }
    }

    return false;
}

[[nodiscard]] const mc::BlockState* getDeadBlockState(mc::u32 deadBlockId)
{
    mc::Block* deadBlock = mc::BlockRegistry::instance().getBlock(deadBlockId);
    return deadBlock != nullptr ? &deadBlock->defaultState() : nullptr;
}

} // namespace

// ========== CoralPlantBlock ==========

CoralPlantBlock::CoralPlantBlock(CoralColor color, u32 deadBlock, const BlockProperties& properties)
    : Block(properties)
    , m_color(color)
    , m_deadBlock(deadBlock)
{
    // 创建状态容器（仅 waterlogged，对齐 vanilla BaseCoralPlantTypeBlock）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::WATERLOGGED())
            .create([this](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态（vanilla 默认 waterlogged=true）
    setDefaultState(defaultState().with(BlockStateProperties::WATERLOGGED(), true));
}

BlockState CoralPlantBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    return defaultState().with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool CoralPlantBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    // 与 vanilla BaseCoralPlantTypeBlock.canSurvive 一致：检查下方块顶面是否坚固
    return canAttachTo(world, pos, Direction::Down);
}

BlockState CoralPlantBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 下方支撑失效则移除（vanilla CoralPlantBlock.updateShape: facing==DOWN && !canSurvive -> AIR）
    if (facing == Direction::Down && !canAttachTo(static_cast<IBlockReader&>(world), currentPos, Direction::Down)) {
        if (auto* airState = BlockRegistry::instance().airState()) {
            return *airState;
        }
    }

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    // 离水则调度死亡（延迟 60~99 tick 后转死珊瑚），而非立即替换——对齐 vanilla tryScheduleDieTick。
    if (!hasNearbyWater(world, currentPos)) {
        world.tickManager().scheduleBlockTick(currentPos, *this, 60 + world.getRandom().nextInt(40));
    }

    return state;
}

void CoralPlantBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston)
{
    MC_UNUSED(state);
    MC_UNUSED(movedByPiston);
    // 放置时若无水，调度死亡（对齐 vanilla CoralPlantBlock.onPlace -> tryScheduleDieTick）
    if (!hasNearbyWater(world, pos)) {
        world.tickManager().scheduleBlockTick(pos, *this, 60 + world.getRandom().nextInt(40));
    }
}

void CoralPlantBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    // 计划刻到期：仍无水则转死珊瑚（对齐 vanilla CoralPlantBlock.tick）
    if (!hasNearbyWater(world, pos)) {
        if (const BlockState* deadState = getDeadBlockState(m_deadBlock); deadState != nullptr) {
            world.setBlockState(pos, deadState, world::BlockUpdateFlags::UPDATE_CLIENTS);
        }
    }
}

const CollisionShape& CoralPlantBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 珊瑚植物是 12x12 的柱状（高 15/16），对齐 vanilla Block.column(12.0, 0.0, 15.0)
    static CollisionShape shape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 0.9375f, 0.875f);
    return shape;
}

bool CoralPlantBlock::canAttachTo(IBlockReader& world, const BlockPos& pos, Direction direction) const
{
    BlockPos adjPos = pos.offset(direction);
    const BlockState* adjState = world.getBlockState(adjPos);

    if (adjState == nullptr) {
        return false;
    }

    // 同 CoralFanBlock：检查底座朝向珊瑚的那一面（direction 的反方向），且用
    // isFaceSturdy(FULL) 而非 isSolidSide。
    return adjState->isFaceSturdy(
        static_cast<IWorld&>(world), adjPos, Directions::opposite(direction), SupportType::Full);
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* CoralPlantBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

} // namespace blocks
} // namespace mc
