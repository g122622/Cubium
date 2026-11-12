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

#include "common/world/block/blocks/dirt/MyceliumBlock.hpp"

#include "common/particle/ParticleTypes.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/capability/IBlockAnimateContext.hpp"
#include "common/world/block/core/Block.hpp"

namespace mc {
namespace blocks {

// MyceliumBlock 实现
// ============================================================================

MyceliumBlock::MyceliumBlock(BlockProperties properties)
    : SpreadableSnowyDirtBlock(std::move(properties))
{}

void MyceliumBlock::animateTick(
    IBlockAnimateContext& context, const BlockPos& pos, const BlockState& state, math::IRandom& random) const
{
    MC_UNUSED(state);
    // MC 原版: 1/10 概率在菌丝方块上方生成菌丝粒子（SuspendedTownParticle）
    if (random.nextInt(10) == 0) {
        f32 x = static_cast<f32>(pos.x) + random.nextFloat();
        f32 y = static_cast<f32>(pos.y) + 1.1f;
        f32 z = static_cast<f32>(pos.z) + random.nextFloat();
        context.addAnimateParticle(particle::ParticleTypeId::Mycelium, Vector3(x, y, z), Vector3(0.0f, 0.0f, 0.0f));
    }
}

} // namespace blocks
} // namespace mc
