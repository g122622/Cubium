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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE ON AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "SporeBlossomBlock.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

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

    // 检查上方是否有坚固面的方块
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState == nullptr) {
        return false;
    }

    // 上方方块必须是实心的（有向下的坚固面）
    return aboveState->isSolid();
}

void SporeBlossomBlock::animateTick(IWorld& world, const BlockPos& pos, const BlockState& state, math::IRandom& random)
{
    MC_UNUSED(state);

    // 生成掉落孢子粒子：从花正下方掉落
    // MC中每tick 2个粒子概率掉落
    if (random.nextInt(5) == 0) {
        f32 x = static_cast<f32>(pos.x) + 0.5f + (random.nextFloat() - 0.5f) * 0.8f;
        f32 y = static_cast<f32>(pos.y) - 0.1f;
        f32 z = static_cast<f32>(pos.z) + 0.5f + (random.nextFloat() - 0.5f) * 0.8f;
        world.addParticle(client::renderer::trident::particle::ParticleTypeId::FallingSporeBlossom,
            Vector3(x, y, z),
            Vector3(0.0f, -0.01f, 0.0f));
    }

    // 生成空气漂浮粒子：在花周围21x10x21区域内漂浮
    if (random.nextInt(10) == 0) {
        // 在花下方 1-10 格、水平 10 格范围内随机位置
        f32 x = static_cast<f32>(pos.x) + random.nextFloat() * 21.0f - 10.0f;
        f32 y = static_cast<f32>(pos.y) - random.nextFloat() * 10.0f;
        f32 z = static_cast<f32>(pos.z) + random.nextFloat() * 21.0f - 10.0f;
        world.addParticle(client::renderer::trident::particle::ParticleTypeId::SporeBlossomAir,
            Vector3(x, y, z),
            Vector3(0.0f, 0.0f, 0.0f));
    }
}

} // namespace blocks
} // namespace mc
