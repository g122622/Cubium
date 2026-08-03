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

#include "Feature.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/DeepslateBlocks.hpp"
#include "common/world/block/registry/TuffBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace mc {

// ============================================================================
// AlwaysTrueRuleTest 实现
// ============================================================================

const AlwaysTrueRuleTest AlwaysTrueRuleTest::INSTANCE;

bool AlwaysTrueRuleTest::test(const BlockState& state, math::Random& random) const
{
    (void)state;
    (void)random;
    return true;
}

std::unique_ptr<RuleTest> AlwaysTrueRuleTest::clone() const
{
    return std::make_unique<AlwaysTrueRuleTest>();
}

// ============================================================================
// BlockMatchRuleTest 实现
// ============================================================================

BlockMatchRuleTest::BlockMatchRuleTest(const Block* block)
    : m_block(block)
{}

bool BlockMatchRuleTest::test(const BlockState& state, math::Random& random) const
{
    (void)random;
    if (!m_block) return false;
    return state.is(m_block);
}

std::unique_ptr<RuleTest> BlockMatchRuleTest::clone() const
{
    return std::make_unique<BlockMatchRuleTest>(m_block);
}

// ============================================================================
// BlockStateMatchRuleTest 实现
// ============================================================================

BlockStateMatchRuleTest::BlockStateMatchRuleTest(const BlockState* state)
    : m_state(state)
{}

bool BlockStateMatchRuleTest::test(const BlockState& state, math::Random& random) const
{
    (void)random;
    if (!m_state) return false;
    // 方块状态完全匹配需要检查 stateId
    return state.stateId() == m_state->stateId();
}

std::unique_ptr<RuleTest> BlockStateMatchRuleTest::clone() const
{
    return std::make_unique<BlockStateMatchRuleTest>(m_state);
}

// ============================================================================
// RandomBlockMatchRuleTest 实现
// ============================================================================

RandomBlockMatchRuleTest::RandomBlockMatchRuleTest(const Block* block, f32 probability)
    : m_block(block)
    , m_probability(probability)
{}

bool RandomBlockMatchRuleTest::test(const BlockState& state, math::Random& random) const
{
    if (!m_block) return false;
    return state.is(m_block) && random.nextFloat() < m_probability;
}

std::unique_ptr<RuleTest> RandomBlockMatchRuleTest::clone() const
{
    return std::make_unique<RandomBlockMatchRuleTest>(m_block, m_probability);
}

// ============================================================================
// RandomBlockStateMatchRuleTest 实现
// ============================================================================

RandomBlockStateMatchRuleTest::RandomBlockStateMatchRuleTest(const BlockState* state, f32 probability)
    : m_state(state)
    , m_probability(probability)
{}

bool RandomBlockStateMatchRuleTest::test(const BlockState& state, math::Random& random) const
{
    if (!m_state) return false;
    return state.stateId() == m_state->stateId() && random.nextFloat() < m_probability;
}

std::unique_ptr<RuleTest> RandomBlockStateMatchRuleTest::clone() const
{
    return std::make_unique<RandomBlockStateMatchRuleTest>(m_state, m_probability);
}

// ============================================================================
// TagMatchRuleTest 实现
// ============================================================================

TagMatchRuleTest::TagMatchRuleTest(const ResourceLocation& tagId)
    : m_tagId(tagId)
{}

bool TagMatchRuleTest::test(const BlockState& state, math::Random& random) const
{
    (void)random;
    // 查询 BlockTags 并检查方块是否在标签中
    BlockTag* tag = BlockTags::getTag(m_tagId);
    if (!tag) {
        return false;
    }
    return tag->contains(state);
}

std::unique_ptr<RuleTest> TagMatchRuleTest::clone() const
{
    return std::make_unique<TagMatchRuleTest>(m_tagId);
}

// ============================================================================
// StoneRuleTest 实现
// ============================================================================

bool StoneRuleTest::test(const BlockState& state, math::Random& random) const
{
    (void)random;
    // 匹配石头、花岗岩、闪长岩、安山岩
    return state.is(VanillaBlocks::STONE) || state.is(VanillaBlocks::GRANITE) || state.is(VanillaBlocks::DIORITE) ||
        state.is(VanillaBlocks::ANDESITE);
}

std::unique_ptr<RuleTest> StoneRuleTest::clone() const
{
    return std::make_unique<StoneRuleTest>();
}

// ============================================================================
// DeepslateRuleTest 实现
// ============================================================================

bool DeepslateRuleTest::test(const BlockState& state, math::Random& random) const
{
    (void)random;
    // 匹配深板岩和凝灰岩（MC 1.21: DEEPSLATE_ORE_REPLACEABLES 标签）
    if (block_registry::DeepslateBlocks::DEEPSLATE && state.is(block_registry::DeepslateBlocks::DEEPSLATE)) {
        return true;
    }
    if (block_registry::TuffBlocks::TUFF && state.is(block_registry::TuffBlocks::TUFF)) {
        return true;
    }
    return false;
}

std::unique_ptr<RuleTest> DeepslateRuleTest::clone() const
{
    return std::make_unique<DeepslateRuleTest>();
}

// ============================================================================
// OreFeatureConfig 实现
// ============================================================================

OreFeatureConfig::OreFeatureConfig(std::vector<OreTarget> oreTargets, i32 veinSize, f32 discardChance)
    : targets(std::move(oreTargets))
    , size(veinSize)
    , discardChanceOnAirExposure(discardChance)
{}

OreFeatureConfig::OreFeatureConfig(
    std::unique_ptr<RuleTest> targetRule, const BlockState* oreState, i32 veinSize, f32 discardChance)
    : size(veinSize)
    , discardChanceOnAirExposure(discardChance)
{
    if (targetRule && oreState) {
        targets.emplace_back(std::move(targetRule), oreState);
    }
}

std::unique_ptr<RuleTest> OreFeatureConfig::naturalStone()
{
    return std::make_unique<StoneRuleTest>();
}

std::unique_ptr<RuleTest> OreFeatureConfig::deepslateStone()
{
    return std::make_unique<DeepslateRuleTest>();
}

std::vector<OreTarget> OreFeatureConfig::stoneAndDeepslateOre(
    const BlockState* stoneOre, const BlockState* deepslateOre)
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

// ============================================================================
// createOreTarget 实现
// ============================================================================

std::unique_ptr<RuleTest> createOreTarget(OreTargetType type)
{
    switch (type) {
        case OreTargetType::NaturalStone:
            return std::make_unique<StoneRuleTest>();
        case OreTargetType::Deepslate:
            return std::make_unique<DeepslateRuleTest>();
        case OreTargetType::Netherrack:
            if (VanillaBlocks::NETHERRACK) {
                return std::make_unique<BlockMatchRuleTest>(VanillaBlocks::NETHERRACK);
            }
            return std::make_unique<StoneRuleTest>();
        case OreTargetType::Basalt:
            if (VanillaBlocks::BASALT) {
                return std::make_unique<BlockMatchRuleTest>(VanillaBlocks::BASALT);
            }
            return std::make_unique<StoneRuleTest>();
        default:
            return std::make_unique<StoneRuleTest>();
    }
}

} // namespace mc
