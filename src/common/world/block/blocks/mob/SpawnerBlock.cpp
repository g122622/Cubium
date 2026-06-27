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

#include "SpawnerBlock.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"
#include "common/world/blockentity/spawner/MobSpawnerBlockEntity.hpp"

namespace mc {
namespace blocks {

SpawnerBlock::SpawnerBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 刷怪笼没有特殊状态
}

ActionResultType SpawnerBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // TODO: 实现创造模式下打开刷怪笼编辑界面
    return ActionResultType::Pass;
}

std::unique_ptr<BlockEntity> SpawnerBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::MobSpawnerBlockEntity>(pos);
}

void SpawnerBlock::animateTick(
    IBlockAnimateContext& context, const BlockPos& pos, const BlockState& state, math::IRandom& random) const
{
    MC_UNUSED(state);

    // 参考 MC: BaseSpawner.clientTick()，在刷怪笼方块内随机位置生成烟雾和火焰粒子
    f32 x = static_cast<f32>(pos.x) + random.nextFloat();
    f32 y = static_cast<f32>(pos.y) + random.nextFloat();
    f32 z = static_cast<f32>(pos.z) + random.nextFloat();

    context.addAnimateParticle(particle::ParticleTypeId::Smoke, Vector3(x, y, z), Vector3(0.0f, 0.0f, 0.0f));
    context.addAnimateParticle(particle::ParticleTypeId::Flame, Vector3(x, y, z), Vector3(0.0f, 0.0f, 0.0f));
}

} // namespace blocks
} // namespace mc
