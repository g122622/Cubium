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

#include "SporeBlossomBlock.hpp"
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

SporeBlossomBlock::SporeBlossomBlock(const BlockProperties& properties)
    : Block(properties)
    , m_shape(CollisionShape::fromPixelBox(2, 13, 2, 14, 16, 14))
{}

const CollisionShape& SporeBlossomBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

bool SporeBlossomBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    // 与 MC 1.21.11 SporeBlossomBlock.canSurvive 一致：
    //   Block.canSupportCenter(world, pos.above(), Direction.DOWN) && !world.isWaterAt(pos)
    if (!Block::canSupportCenter(world, pos.offset(Direction::Up), Direction::Down)) {
        return false;
    }
    return !world.isWaterAt(pos);
}

BlockState SporeBlossomBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // MC 1.21.11: 当上方方块变化时，如果不再满足 canSurvive 则变为空气
    if (facing == Direction::Up && !isValidPosition(state, static_cast<IBlockReader&>(world), currentPos)) {
        // 返回空气方块默认状态
        if (auto* airBlock = VanillaBlocks::AIR) {
            return airBlock->defaultState();
        }
        return Block::defaultState();
    }
    return Block::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

void SporeBlossomBlock::animateTick(
    IBlockAnimateContext& context, const BlockPos& pos, const BlockState& state, math::IRandom& random) const
{
    MC_UNUSED(state);

    // MC 1.21.11 对齐：falling_spore_blossom 粒子从花底部中心附近掉落
    const f32 x = static_cast<f32>(pos.x) + 0.5f + (random.nextFloat() - 0.5f) * 0.8f;
    const f32 y = static_cast<f32>(pos.y) + 0.7f;
    const f32 z = static_cast<f32>(pos.z) + 0.5f + (random.nextFloat() - 0.5f) * 0.8f;
    context.addAnimateParticle(
        particle::ParticleTypeId::FallingSporeBlossom, Vector3(x, y, z), Vector3(0.0f, 0.0f, 0.0f));

    // MC 1.21.11 对齐：spore_blossom_air 粒子在花周围尝试14次
    // 在 xz[-10,10] y[-10,0] 范围内随机选点，只在非完整碰撞箱的位置生成
    for (int i = 0; i < 14; ++i) {
        const int px = pos.x + random.nextInt(21) - 10;
        const int py = pos.y - random.nextInt(10);
        const int pz = pos.z + random.nextInt(21) - 10;
        const BlockState* blockState = context.getBlockState(px, py, pz);
        if (blockState && !blockState->isSolid()) {
            const f32 ppx = static_cast<f32>(px) + random.nextFloat();
            const f32 ppy = static_cast<f32>(py) + random.nextFloat();
            const f32 ppz = static_cast<f32>(pz) + random.nextFloat();
            context.addAnimateParticle(
                particle::ParticleTypeId::SporeBlossomAir, Vector3(ppx, ppy, ppz), Vector3(0.0f, 0.0f, 0.0f));
        }
    }
}

} // namespace blocks
} // namespace mc
