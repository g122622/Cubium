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
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/surface/SurfaceRuleContext.hpp"
#include "common/util/math/MathUtils.hpp"

#include <algorithm>
#include <climits>

namespace mc::world::gen::surface {

// ============================================================================
// StoneDepthCondition — MC: StoneDepthCheck
// MC 逻辑: stoneDepth <= 1 + offset + (addSurfaceDepth ? surfaceDepth : 0) + secondaryDepthRange映射
// ============================================================================

bool StoneDepthCondition::test(const SurfaceRuleContext& ctx) const
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

bool YCondition::test(const SurfaceRuleContext& ctx) const
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

bool WaterCondition::test(const SurfaceRuleContext& ctx) const
{
    if (ctx.waterHeight() == INT_MIN) {
        return true;
    }
    const i32 y = ctx.blockY() + (m_addStoneDepth ? ctx.stoneDepthAbove() : 0);
    return y >= ctx.waterHeight() + m_offset + ctx.surfaceDepth() * m_surfaceDepthMultiplier;
}

// ============================================================================
// BiomeCondition
// ============================================================================

bool BiomeCondition::test(const SurfaceRuleContext& ctx) const
{
    return std::find(m_biomes.begin(), m_biomes.end(), ctx.biome()) != m_biomes.end();
}

// ============================================================================
// NoiseThresholdCondition — MC: NoiseThresholdConditionSource
// ============================================================================

bool NoiseThresholdCondition::test(const SurfaceRuleContext& ctx) const
{
    // MC 1.21: 通过 RandomState 查找噪声实例
    auto& noise = ctx.randomState()->getOrCreateNoise(m_noiseName);
    const f64 value = noise.getValue(static_cast<f64>(ctx.blockX()), 0.0, static_cast<f64>(ctx.blockZ()));
    return value >= m_minThreshold && value <= m_maxThreshold;
}

// ============================================================================
// VerticalGradientCondition — MC: VerticalGradientConditionSource
// 用于基岩层等（随机梯度过渡）
// ============================================================================

bool VerticalGradientCondition::test(const SurfaceRuleContext& ctx) const
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

    // MC 1.21: 使用 PositionalRandomFactory.at(x, y, z).nextFloat()
    auto& factory = ctx.randomState()->getOrCreateRandomFactory(m_randomName);
    auto rng = factory.at(ctx.blockX(), blockY, ctx.blockZ());
    const f64 chance = static_cast<f64>(rng->nextFloat());
    const f64 threshold = static_cast<f64>(falseY - blockY) / static_cast<f64>(falseY - trueY);
    return chance < threshold;
}

// ============================================================================
// SteepCondition, TemperatureCondition, HoleCondition, AbovePreliminarySurfaceCondition
// ============================================================================

bool SteepCondition::test(const SurfaceRuleContext& ctx) const
{
    return ctx.steep();
}

bool TemperatureCondition::test(const SurfaceRuleContext& ctx) const
{
    return ctx.temperature();
}

bool HoleCondition::test(const SurfaceRuleContext& ctx) const
{
    return ctx.hole();
}

bool AbovePreliminarySurfaceCondition::test(const SurfaceRuleContext& ctx) const
{
    return ctx.abovePreliminarySurface();
}

} // namespace mc::world::gen::surface
