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

#include "WallTorchBlock.hpp"

#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"
#include "common/world/block/SupportType.hpp"
#include "common/world/block/blocks/decorative/TorchBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

WallTorchBlock::WallTorchBlock(const BlockProperties& properties, particle::ParticleTypeId flameParticle)
    : TorchBlock(properties, flameParticle)
    , m_northShape(CollisionShape::fromPixelBox(5.5f, 3.0f, 11.0f, 10.5f, 13.0f, 16.0f))
    , m_southShape(CollisionShape::fromPixelBox(5.5f, 3.0f, 0.0f, 10.5f, 13.0f, 5.0f))
    , m_westShape(CollisionShape::fromPixelBox(11.0f, 3.0f, 5.5f, 16.0f, 13.0f, 10.5f))
    , m_eastShape(CollisionShape::fromPixelBox(0.0f, 3.0f, 5.5f, 5.0f, 13.0f, 10.5f))
{
    // 创建状态容器，添加 HORIZONTAL_FACING 属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));
}

const CollisionShape& WallTorchBlock::getShape(const BlockState& state) const
{
    switch (getFacing(state)) {
        case Direction::North:
            return m_northShape;
        case Direction::South:
            return m_southShape;
        case Direction::West:
            return m_westShape;
        case Direction::East:
            return m_eastShape;
        default:
            return m_northShape;
    }
}

Direction WallTorchBlock::getFacing(const BlockState& state)
{
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

const BlockState& WallTorchBlock::withFacing(const BlockState& state, Direction facing)
{
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), facing);
}

bool WallTorchBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 与 MC 1.21.11 WallTorchBlock.canSurvive 一致：
    //   blockstate.isFaceSturdy(world, blockpos, facing)（默认 SupportType.FULL）
    const Direction facing = getFacing(state);
    const Direction attachDir = Directions::opposite(facing);
    const BlockPos attachPos = pos.offset(attachDir);
    const BlockState* attachState = world.getBlockState(attachPos);
    if (!attachState || attachState->isAir()) {
        return false;
    }
    return attachState->isFaceSturdy(world, attachPos, facing, SupportType::Full);
}

bool WallTorchBlock::_canPlaceAt(IWorld& world, const BlockPos& pos, Direction facing) const
{
    const Direction attachDir = Directions::opposite(facing);
    const BlockPos attachPos = pos.offset(attachDir);
    const BlockState* attachState = world.getBlockState(attachPos);
    if (!attachState || attachState->isAir()) {
        return false;
    }
    return attachState->isFaceSturdy(world, attachPos, facing, SupportType::Full);
}

BlockState WallTorchBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(currentPos);

    // 与 MC 1.21.11 一致：附着方向邻居变化且 canSurvive 失败时变为空气
    const Direction torchFacing = getFacing(state);
    if (facing == Directions::opposite(torchFacing)) {
        if (!facingState.isAir() && facingState.isFaceSturdy(world, facingPos, torchFacing, SupportType::Full)) {
            return state;
        }
        if (auto* airBlock = VanillaBlocks::AIR) {
            return airBlock->defaultState();
        }
        return Block::defaultState();
    }

    return Block::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

BlockState WallTorchBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 与 MC 1.21.11 一致：遍历玩家视线方向，仅水平方向有效
    BlockState blockstate = defaultState();
    IWorld& world = context.getWorld();
    const BlockPos pos = context.placementPos();

    for (const Direction direction : context.getNearestLookingDirections()) {
        if (!Directions::isHorizontal(direction)) {
            continue;
        }
        // 与 MC 1.21.11 WallTorchBlock.getStateForPlacement 一致：
        //   direction1 = direction.getOpposite()（火把朝向 FACING）
        //   canSurvive: attachPos = torchPos.relative(direction)（注意是 direction，而非 direction1）
        //               isFaceSturdy(world, attachPos, facing=direction1)
        const Direction facing = Directions::opposite(direction);
        blockstate = blockstate.with(BlockStateProperties::HORIZONTAL_FACING(), facing);
        const BlockPos attachPos = pos.offset(direction);
        const BlockState* attachState = world.getBlockState(attachPos);
        if (attachState && !attachState->isAir() &&
            attachState->isFaceSturdy(world, attachPos, facing, SupportType::Full)) {
            return blockstate;
        }
    }

    return defaultState();
}

void WallTorchBlock::animateTick(
    IBlockAnimateContext& context, const BlockPos& pos, const BlockState& state, math::IRandom& random) const
{
    MC_UNUSED(random);

    Direction facing = getFacing(state);
    Direction oppositeDir = Directions::opposite(facing);

    f32 x = static_cast<f32>(pos.x) + 0.5f;
    f32 y = static_cast<f32>(pos.y) + 0.7f;
    f32 z = static_cast<f32>(pos.z) + 0.5f;

    // 根据朝向偏移粒子位置
    f32 offsetX = 0.27f * static_cast<f32>(Directions::xOffset(oppositeDir));
    f32 offsetZ = 0.27f * static_cast<f32>(Directions::zOffset(oppositeDir));
    f32 offsetY = 0.22f;

    // 烟雾粒子
    context.addAnimateParticle(
        particle::ParticleTypeId::Smoke, Vector3(x + offsetX, y + offsetY, z + offsetZ), Vector3(0.0f, 0.0f, 0.0f));

    // 火焰粒子
    context.addAnimateParticle(
        m_flameParticle, Vector3(x + offsetX, y + offsetY, z + offsetZ), Vector3(0.0f, 0.0f, 0.0f));
}

const BlockState& WallTorchBlock::rotate(const BlockState& state, Rotation rotation) const
{
    return withFacing(state, Directions::rotateDirection(getFacing(state), rotation));
}

const BlockState& WallTorchBlock::mirror(const BlockState& state, Mirror mirror) const
{
    return rotate(state, Directions::mirrorToRotation(mirror, getFacing(state)));
}

} // namespace blocks
} // namespace mc
