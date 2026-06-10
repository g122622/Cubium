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

#include "RuleTest.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace ruletest {

// ============================================================================
// AlwaysTrueRuleTest
// ============================================================================

const AlwaysTrueRuleTest AlwaysTrueRuleTest::INSTANCE;

bool AlwaysTrueRuleTest::test(const BlockState& /*state*/, math::Random& /*random*/) const
{
    return true;
}

std::unique_ptr<RuleTest> AlwaysTrueRuleTest::clone() const
{
    return std::make_unique<AlwaysTrueRuleTest>();
}

// ============================================================================
// BlockMatchRuleTest
// ============================================================================

BlockMatchRuleTest::BlockMatchRuleTest(const Block* block)
    : m_block(block)
{}

bool BlockMatchRuleTest::test(const BlockState& state, math::Random& /*random*/) const
{
    if (!m_block) return false;
    return state.is(m_block);
}

std::unique_ptr<RuleTest> BlockMatchRuleTest::clone() const
{
    return std::make_unique<BlockMatchRuleTest>(m_block);
}

// ============================================================================
// BlockStateMatchRuleTest
// ============================================================================

BlockStateMatchRuleTest::BlockStateMatchRuleTest(const BlockState* state)
    : m_state(state)
{}

bool BlockStateMatchRuleTest::test(const BlockState& state, math::Random& /*random*/) const
{
    if (!m_state) return false;
    return state.stateId() == m_state->stateId();
}

std::unique_ptr<RuleTest> BlockStateMatchRuleTest::clone() const
{
    return std::make_unique<BlockStateMatchRuleTest>(m_state);
}

// ============================================================================
// RandomBlockMatchRuleTest
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
// RandomBlockStateMatchRuleTest
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
// TagMatchRuleTest
// ============================================================================

TagMatchRuleTest::TagMatchRuleTest(const std::string& tagName)
    : m_tagName(tagName)
{}

TagMatchRuleTest::TagMatchRuleTest(const ResourceLocation& tagId)
    : m_tagName(tagId.toString())
{}

bool TagMatchRuleTest::test(const BlockState& state, math::Random& /*random*/) const
{
    ResourceLocation tagId(m_tagName);
    BlockTag* tag = BlockTags::getTag(tagId);
    if (!tag) {
        return false;
    }
    return tag->contains(state);
}

std::unique_ptr<RuleTest> TagMatchRuleTest::clone() const
{
    return std::make_unique<TagMatchRuleTest>(m_tagName);
}

} // namespace ruletest
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
