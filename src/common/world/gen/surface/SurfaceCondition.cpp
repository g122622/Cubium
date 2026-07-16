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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "common/world/gen/surface/SurfaceCondition.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"
#include "common/world/gen/surface/SurfaceRuleContext.hpp"

#include <algorithm>
#include <climits>
#include <limits>
#include <mutex>

namespace mc::world::gen::surface {

// ============================================================================
// LazyXZCondition / LazyYCondition — MC 1.21 SurfaceRules.LazyCondition
// test() 为 final：委托给 SurfaceRuleContext 的 per-call 缓存（key = this 指针）。
// 命中缓存直接返回；未命中则调用子类 compute() 并写入缓存。
// ============================================================================

bool LazyXZCondition::test(const SurfaceRuleContext& ctx) const
{
    return ctx.cachedXZ(this, *this);
}

bool LazyYCondition::test(const SurfaceRuleContext& ctx) const
{
    return ctx.cachedY(this, *this);
}

// ============================================================================
// StoneDepthCondition — MC: StoneDepthCheck
// MC 逻辑: stoneDepth <= 1 + offset + (addSurfaceDepth ? surfaceDepth : 0) + secondaryDepthRange映射
// ============================================================================

bool StoneDepthCondition::compute(const SurfaceRuleContext& ctx) const
{
    const i32 stoneDepth = (m_surface == CaveSurface::Floor) ? ctx.stoneDepthAbove() : ctx.stoneDepthBelow();
    const i32 surfaceDepthOffset = m_addSurfaceDepth ? ctx.surfaceDepth() : 0;

    const i32 secondaryOffset = (m_secondaryDepthRange == 0)
        ? 0
        : static_cast<i32>(math::map(ctx.surfaceSecondary(), -1.0, 1.0, 0.0, static_cast<f64>(m_secondaryDepthRange)));

    return stoneDepth <= 1 + m_offset + surfaceDepthOffset + secondaryOffset;
}

// ============================================================================
// YCondition — MC: YConditionSource
// MC 逻辑: blockY + (addStoneDepth ? stoneDepthAbove : 0) >= anchorY + surfaceDepth * multiplier
// ============================================================================

bool YCondition::compute(const SurfaceRuleContext& ctx) const
{
    const i32 anchorY = m_anchor.resolveY(ctx.minY(), ctx.height());
    const i32 y = ctx.blockY() + (m_addStoneDepth ? ctx.stoneDepthAbove() : 0);
    return y >= anchorY + ctx.surfaceDepth() * m_surfaceDepthMultiplier;
}

// ============================================================================
// WaterCondition — MC: WaterConditionSource
// MC 逻辑: waterHeight == MIN_VALUE || blockY + (addStoneDepth ? stoneDepthAbove : 0) >= waterHeight + offset +
// surfaceDepth * multiplier
// ============================================================================

bool WaterCondition::compute(const SurfaceRuleContext& ctx) const
{
    if (ctx.waterHeight() == std::numeric_limits<int>::min()) {
        return true;
    }
    const i32 y = ctx.blockY() + (m_addStoneDepth ? ctx.stoneDepthAbove() : 0);
    return y >= ctx.waterHeight() + m_offset + ctx.surfaceDepth() * m_surfaceDepthMultiplier;
}

// ============================================================================
// BiomeCondition
// ============================================================================

bool BiomeCondition::compute(const SurfaceRuleContext& ctx) const
{
    return std::find(m_biomes.begin(), m_biomes.end(), ctx.biome()) != m_biomes.end();
}

// ============================================================================
// NoiseThresholdCondition — MC: NoiseThresholdConditionSource (LazyXZCondition)
// 首次 compute 时通过 std::call_once 解析 NormalNoise* 并缓存，消除热路径字符串查找。
// ============================================================================

bool NoiseThresholdCondition::compute(const SurfaceRuleContext& ctx) const
{
    std::call_once(
        m_resolveOnce, [this, &ctx]() { m_cachedNoise = &ctx.randomState()->getOrCreateNoise(m_noiseName); });
    const f64 value = m_cachedNoise->getValue(static_cast<f64>(ctx.blockX()), 0.0, static_cast<f64>(ctx.blockZ()));
    return value >= m_minThreshold && value <= m_maxThreshold;
}

// ============================================================================
// VerticalGradientCondition — MC: VerticalGradientConditionSource (LazyYCondition)
// 用于基岩层等（随机梯度过渡）。首次 compute 时缓存 PositionalRandomFactory*。
// ============================================================================

bool VerticalGradientCondition::compute(const SurfaceRuleContext& ctx) const
{
    const i32 trueY = m_trueAtAndBelow.resolveY(ctx.minY(), ctx.height());
    const i32 falseY = m_falseAtAndAbove.resolveY(ctx.minY(), ctx.height());
    const i32 blockY = ctx.blockY();

    if (blockY <= trueY) {
        return true;
    }
    if (blockY >= falseY) {
        return false;
    }

    std::call_once(m_resolveOnce,
        [this, &ctx]() { m_cachedFactory = &ctx.randomState()->getOrCreateRandomFactory(m_randomName); });
    // MC 1.21: 使用 PositionalRandomFactory.at(x, y, z).nextFloat()
    auto rng = m_cachedFactory->at(ctx.blockX(), blockY, ctx.blockZ());
    const f64 chance = static_cast<f64>(rng->nextFloat());
    const f64 threshold = static_cast<f64>(falseY - blockY) / static_cast<f64>(falseY - trueY);
    return chance < threshold;
}

// ============================================================================
// SteepCondition, TemperatureCondition, HoleCondition, AbovePreliminarySurfaceCondition
// ============================================================================

bool SteepCondition::compute(const SurfaceRuleContext& ctx) const
{
    return ctx.steep();
}

bool TemperatureCondition::compute(const SurfaceRuleContext& ctx) const
{
    return ctx.temperature();
}

bool HoleCondition::compute(const SurfaceRuleContext& ctx) const
{
    return ctx.hole();
}

bool AbovePreliminarySurfaceCondition::test(const SurfaceRuleContext& ctx) const
{
    return ctx.abovePreliminarySurface();
}

} // namespace mc::world::gen::surface
