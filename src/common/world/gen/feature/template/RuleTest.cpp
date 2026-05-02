#include "RuleTest.hpp"
#include "../../../block/BlockRegistry.hpp"
#include "../../../block/Block.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

// ============================================================================
// BlockMatchRuleTest
// ============================================================================

BlockMatchRuleTest::BlockMatchRuleTest(u32 blockId)
    : m_blockId(blockId)
{
}

bool BlockMatchRuleTest::test(const BlockState* state, math::Random& /*rng*/) const {
    if (!state) {
        return false;
    }
    // 检查方块ID是否匹配
    return state->blockId() == m_blockId;
}

// ============================================================================
// BlockStateMatchRuleTest
// ============================================================================

BlockStateMatchRuleTest::BlockStateMatchRuleTest(u32 stateId)
    : m_stateId(stateId)
{
}

bool BlockStateMatchRuleTest::test(const BlockState* state, math::Random& /*rng*/) const {
    if (!state) {
        return false;
    }
    // 检查完整状态ID是否匹配
    return state->stateId() == m_stateId;
}

// ============================================================================
// RandomBlockMatchRuleTest
// ============================================================================

RandomBlockMatchRuleTest::RandomBlockMatchRuleTest(u32 blockId, f32 probability)
    : m_blockId(blockId)
    , m_probability(probability)
{
}

bool RandomBlockMatchRuleTest::test(const BlockState* state, math::Random& rng) const {
    if (!state) {
        return false;
    }
    // 先检查方块ID
    if (state->blockId() != m_blockId) {
        return false;
    }
    // 随机决定
    return rng.nextFloat() < m_probability;
}

// ============================================================================
// RandomBlockStateMatchRuleTest
// ============================================================================

RandomBlockStateMatchRuleTest::RandomBlockStateMatchRuleTest(u32 stateId, f32 probability)
    : m_stateId(stateId)
    , m_probability(probability)
{
}

bool RandomBlockStateMatchRuleTest::test(const BlockState* state, math::Random& rng) const {
    if (!state) {
        return false;
    }
    // 先检查状态ID
    if (state->stateId() != m_stateId) {
        return false;
    }
    // 随机决定
    return rng.nextFloat() < m_probability;
}

// ============================================================================
// LinearPosRuleTest
// ============================================================================

LinearPosRuleTest::LinearPosRuleTest(
    i32 minHeight,
    i32 maxHeight,
    f32 minProbability,
    f32 maxProbability)
    : m_minHeight(minHeight)
    , m_maxHeight(maxHeight)
    , m_minProbability(minProbability)
    , m_maxProbability(maxProbability)
{
}

bool LinearPosRuleTest::test(
    const BlockPos& /*originalPos*/,
    const BlockPos& worldPos,
    const BlockPos& /*seedPos*/,
    math::Random& rng) const
{
    // MC 1.16.5: LinearPosRuleTest.func_230385_a_
    // 根据 Y 坐标在 minHeight 和 maxHeight 之间线性插值概率
    if (m_minHeight == m_maxHeight) {
        return rng.nextFloat() < m_minProbability;
    }

    f32 t = static_cast<f32>(worldPos.y - m_minHeight) / static_cast<f32>(m_maxHeight - m_minHeight);
    t = std::clamp(t, 0.0f, 1.0f);
    f32 probability = m_minProbability + t * (m_maxProbability - m_minProbability);

    return rng.nextFloat() < probability;
}

// ============================================================================
// RuleEntry
// ============================================================================

RuleEntry::RuleEntry(
    std::unique_ptr<RuleTest> inputPredicate,
    std::unique_ptr<RuleTest> locationPredicate,
    std::unique_ptr<PosRuleTest> posPredicate,
    u32 outputStateId)
    : m_inputPredicate(std::move(inputPredicate))
    , m_locationPredicate(std::move(locationPredicate))
    , m_posPredicate(std::move(posPredicate))
    , m_outputStateId(outputStateId)
{
}

RuleEntry::RuleEntry(
    std::unique_ptr<RuleTest> inputPredicate,
    std::unique_ptr<RuleTest> locationPredicate,
    u32 outputStateId)
    : RuleEntry(
        std::move(inputPredicate),
        std::move(locationPredicate),
        std::make_unique<AlwaysTruePosRuleTest>(),
        outputStateId)
{
}

bool RuleEntry::matches(
    const BlockState* inputState,
    const BlockState* locationState,
    const BlockPos& originalPos,
    const BlockPos& worldPos,
    const BlockPos& seedPos,
    math::Random& rng) const
{
    // MC 1.16.5: RuleEntry.func_237110_a_
    // 三个条件必须全部满足
    if (m_inputPredicate && !m_inputPredicate->test(inputState, rng)) {
        return false;
    }
    if (m_locationPredicate && !m_locationPredicate->test(locationState, rng)) {
        return false;
    }
    if (m_posPredicate && !m_posPredicate->test(originalPos, worldPos, seedPos, rng)) {
        return false;
    }
    return true;
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
