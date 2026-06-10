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

#include "OreFeatureConfig.hpp"
#include "common/world/gen/feature/ruletest/StoneRuleTest.hpp"
#include "common/world/gen/feature/ruletest/DeepslateRuleTest.hpp"

namespace mc {

// ============================================================================
// OreTarget 实现
// ============================================================================

OreTarget::OreTarget(std::unique_ptr<world::gen::feature::ruletest::RuleTest> targetRule, const BlockState* oreState)
    : target(std::move(targetRule))
    , state(oreState)
{}

// ============================================================================
// OreFeatureConfig 实现
// ============================================================================

OreFeatureConfig::OreFeatureConfig(std::vector<OreTarget> oreTargets, i32 veinSize, f32 discardChance)
    : targets(std::move(oreTargets))
    , size(veinSize)
    , discardChanceOnAirExposure(discardChance)
{}

OreFeatureConfig::OreFeatureConfig(
    std::unique_ptr<world::gen::feature::ruletest::RuleTest> targetRule, const BlockState* oreState, i32 veinSize, f32 discardChance)
    : size(veinSize)
    , discardChanceOnAirExposure(discardChance)
{
    if (targetRule && oreState) {
        targets.emplace_back(std::move(targetRule), oreState);
    }
}

std::unique_ptr<world::gen::feature::ruletest::RuleTest> OreFeatureConfig::naturalStone()
{
    return std::make_unique<world::gen::feature::ruletest::StoneRuleTest>();
}

std::unique_ptr<world::gen::feature::ruletest::RuleTest> OreFeatureConfig::deepslateStone()
{
    return std::make_unique<world::gen::feature::ruletest::DeepslateRuleTest>();
}

std::vector<OreTarget> OreFeatureConfig::stoneAndDeepslateOre(const BlockState* stoneOre, const BlockState* deepslateOre)
{
    std::vector<OreTarget> result;
    result.reserve(2);
    if (stoneOre) {
        result.emplace_back(naturalStone(), stoneOre);
    }
    if (deepslateOre) {
        result.emplace_back(deepslateStone(), deepslateOre);
    }
    return result;
}

} // namespace mc
