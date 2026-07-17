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

#include "common/world/gen/surface/SurfaceRulesFactory.hpp"
#include <utility>

namespace mc::world::gen::surface {

namespace SurfaceRules {

// ============================================================================
// 条件工厂
// ============================================================================

std::unique_ptr<SurfaceCondition> stoneDepthCheck(
    i32 offset, bool addSurfaceDepth, i32 secondaryDepthRange, CaveSurface surface)
{
    return std::make_unique<StoneDepthCondition>(offset, addSurfaceDepth, secondaryDepthRange, surface);
}

std::unique_ptr<SurfaceCondition> isBiome(std::vector<BiomeId> biomes)
{
    return std::make_unique<BiomeCondition>(std::move(biomes));
}

std::unique_ptr<SurfaceCondition> notCondition(std::unique_ptr<SurfaceCondition> condition)
{
    return std::make_unique<NotCondition>(std::move(condition));
}

std::unique_ptr<SurfaceCondition> noiseCondition(std::string noiseName, f64 minThreshold, f64 maxThreshold)
{
    return std::make_unique<NoiseThresholdCondition>(std::move(noiseName), minThreshold, maxThreshold);
}

std::unique_ptr<SurfaceCondition> verticalGradient(
    std::string randomName, VerticalAnchor trueAtAndBelow, VerticalAnchor falseAtAndAbove)
{
    return std::make_unique<VerticalGradientCondition>(std::move(randomName), trueAtAndBelow, falseAtAndAbove);
}

std::unique_ptr<SurfaceCondition> steep()
{
    return std::make_unique<SteepCondition>();
}

std::unique_ptr<SurfaceCondition> temperature()
{
    return std::make_unique<TemperatureCondition>();
}

std::unique_ptr<SurfaceCondition> hole()
{
    return std::make_unique<HoleCondition>();
}

std::unique_ptr<SurfaceCondition> abovePreliminarySurface()
{
    return std::make_unique<AbovePreliminarySurfaceCondition>();
}

// ============================================================================
// 规则工厂
// ============================================================================

std::unique_ptr<SurfaceRule> blockState(const BlockState* state)
{
    return std::make_unique<BlockRule>(state);
}

std::unique_ptr<SurfaceRule> ifTrue(std::unique_ptr<SurfaceCondition> condition, std::unique_ptr<SurfaceRule> thenRule)
{
    return std::make_unique<IfTrueRule>(std::move(condition), std::move(thenRule));
}

std::unique_ptr<SurfaceRule> sequence(std::vector<std::unique_ptr<SurfaceRule>> rules)
{
    return std::make_unique<SequenceRule>(std::move(rules));
}

std::unique_ptr<SurfaceRule> bandlands()
{
    return std::make_unique<BandlandsRule>();
}

} // namespace SurfaceRules

} // namespace mc::world::gen::surface
