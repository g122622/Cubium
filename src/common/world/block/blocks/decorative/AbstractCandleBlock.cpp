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
 * The above copyright notice shall be included in all
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

#include "AbstractCandleBlock.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"
#include <utility>

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

AbstractCandleBlock::AbstractCandleBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 状态容器由子类创建，因为不同子类有不同的属性组合
    // 例如 CandleBlock 有 CANDLES(1-4) + LIT，CandleCakeBlock 只有 LIT
}

// ========== 点燃/熄灭 ==========

bool AbstractCandleBlock::isLit(const BlockState& state)
{
    return state.get(BlockStateProperties::LIT());
}

bool AbstractCandleBlock::canBeLit(const BlockState& state) const
{
    // 未点燃的蜡烛可以被点燃
    return !state.get(BlockStateProperties::LIT());
}

void AbstractCandleBlock::extinguish(IWorld& world, const BlockPos& pos, BlockState& state, Player* player)
{
    MC_UNUSED(player);

    if (isLit(state)) {
        BlockState newState = state.with(BlockStateProperties::LIT(), false);
        world.setBlockState(pos, &newState, 3);

        // 熄灭时播放蜡烛熄灭音效
        if (!world.isClientSide()) {
            world.playSound(
                SoundEvents::BLOCK_CANDLE_EXTINGUISH, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
        }
    }
}

void AbstractCandleBlock::setLit(IWorld& world, const BlockPos& pos, const BlockState& state, bool lit)
{
    BlockState newState = state.with(BlockStateProperties::LIT(), lit);
    world.setBlockState(pos, &newState, 3);
}

// ========== 投掷物交互 ==========

void AbstractCandleBlock::onProjectileHit(
    IWorld& world, const BlockState& state, const BlockRaycastResult& hitResult, Entity& projectile)
{
    // 着火的投掷物击中蜡烛时，如果蜡烛可以被点燃则点燃
    if (!world.isClientSide() && projectile.isOnFire() && canBeLit(state)) {
        setLit(world, hitResult.blockPos(), state, true);
    }
}

// ========== 粒子动画 ==========

void AbstractCandleBlock::animateTick(
    IBlockAnimateContext& context, const BlockPos& pos, const BlockState& state, math::IRandom& random) const
{
    if (isLit(state)) {
        auto offsets = getParticleOffsets(state);
        for (const auto& offset : offsets) {
            f32 x = static_cast<f32>(pos.x) + offset.x;
            f32 y = static_cast<f32>(pos.y) + offset.y;
            f32 z = static_cast<f32>(pos.z) + offset.z;

            // 烟雾粒子（随机生成，约30%概率）
            f32 randVal = random.nextFloat();
            if (randVal < 0.3f) {
                context.addAnimateParticle(
                    particle::ParticleTypeId::Smoke, Vector3(x, y, z), Vector3(0.0f, 0.0f, 0.0f));

                // 蜡烛噼啪声（约17%概率，即总体约5%的概率/tick）
                if (randVal < 0.17f) {
                    context.playLocalSound(SoundEvents::BLOCK_CANDLE_AMBIENT,
                        sound::SoundCategory::Blocks,
                        Vector3(x, y, z),
                        1.0f + random.nextFloat(),
                        random.nextFloat() * 0.7f + 0.3f);
                }
            }

            // 火焰粒子（始终生成）
            context.addAnimateParticle(
                particle::ParticleTypeId::SmallFlame, Vector3(x, y, z), Vector3(0.0f, 0.0f, 0.0f));
        }
    }
}

// ========== 光照 ==========

u8 AbstractCandleBlock::getLightLevel(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 默认实现返回0，子类应根据蜡烛数量和是否点燃覆盖此方法
    // CandleBlock: 点燃时返回 3 * candles
    // CandleCakeBlock: 点燃时返回 3
    return 0;
}

} // namespace blocks
} // namespace mc
