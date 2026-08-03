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

#include "TorchBlock.hpp"

#include "common/core/Types.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace blocks {

TorchBlock::TorchBlock(const BlockProperties& properties, particle::ParticleTypeId flameParticle)
    : Block(properties)
    , m_shape(CollisionShape::fromPixelBox(7.0f, 0.0f, 7.0f, 9.0f, 10.0f, 9.0f))
    , m_flameParticle(flameParticle)
{}

const CollisionShape& TorchBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

bool TorchBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    // 与 MC 1.21.11 BaseTorchBlock.canSurvive 一致：
    //   Block.canSupportCenter(world, pos.below(), Direction.UP)
    return Block::canSupportCenter(world, pos.down(), Direction::Up);
}

BlockState TorchBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 当下方方块变化时，如果不再满足放置条件则变为空气
    if (facing == Direction::Down && !_canSurvive(world, currentPos)) {
        if (auto* airBlock = VanillaBlocks::AIR) {
            return airBlock->defaultState();
        }
        return Block::defaultState();
    }

    return Block::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

bool TorchBlock::_canSurvive(IWorld& world, const BlockPos& pos) const
{
    // 与 MC 1.21.11 BaseTorchBlock.canSurvive 一致：
    //   Block.canSupportCenter(world, pos.below(), Direction.UP)
    return Block::canSupportCenter(world, pos.down(), Direction::Up);
}

void TorchBlock::animateTick(
    IBlockAnimateContext& context, const BlockPos& pos, const BlockState& state, math::IRandom& random) const
{
    MC_UNUSED(random);

    const f32 x = static_cast<f32>(pos.x) + 0.5f;
    const f32 y = static_cast<f32>(pos.y) + 0.7f;
    const f32 z = static_cast<f32>(pos.z) + 0.5f;

    // 烟雾粒子
    context.addAnimateParticle(particle::ParticleTypeId::Smoke, Vector3(x, y, z), Vector3(0.0f, 0.0f, 0.0f));

    // 火焰粒子
    context.addAnimateParticle(m_flameParticle, Vector3(x, y, z), Vector3(0.0f, 0.0f, 0.0f));
}

} // namespace blocks
} // namespace mc
