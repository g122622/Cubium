#include "Feature.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../block/VanillaBlocks.hpp"

namespace mc {

// ============================================================================
// AlwaysTrueRuleTest 实现
// ============================================================================

const AlwaysTrueRuleTest AlwaysTrueRuleTest::INSTANCE;

bool AlwaysTrueRuleTest::test(const BlockState& state, math::Random& random) const {
    (void)state;
    (void)random;
    return true;
}

std::unique_ptr<RuleTest> AlwaysTrueRuleTest::clone() const {
    return std::unique_ptr<RuleTest>(const_cast<AlwaysTrueRuleTest*>(&INSTANCE));
}

// ============================================================================
// BlockMatchRuleTest 实现
// ============================================================================

BlockMatchRuleTest::BlockMatchRuleTest(const Block* block)
    : m_block(block) {}

bool BlockMatchRuleTest::test(const BlockState& state, math::Random& random) const {
    (void)random;
    if (!m_block) return false;
    return state.is(m_block);
}

std::unique_ptr<RuleTest> BlockMatchRuleTest::clone() const {
    return std::make_unique<BlockMatchRuleTest>(m_block);
}

// ============================================================================
// BlockStateMatchRuleTest 实现
// ============================================================================

BlockStateMatchRuleTest::BlockStateMatchRuleTest(const BlockState* state)
    : m_state(state) {}

bool BlockStateMatchRuleTest::test(const BlockState& state, math::Random& random) const {
    (void)random;
    if (!m_state) return false;
    // 方块状态完全匹配需要检查 stateId
    return state.stateId() == m_state->stateId();
}

std::unique_ptr<RuleTest> BlockStateMatchRuleTest::clone() const {
    return std::make_unique<BlockStateMatchRuleTest>(m_state);
}

// ============================================================================
// RandomBlockMatchRuleTest 实现
// ============================================================================

RandomBlockMatchRuleTest::RandomBlockMatchRuleTest(const Block* block, f32 probability)
    : m_block(block)
    , m_probability(probability) {}

bool RandomBlockMatchRuleTest::test(const BlockState& state, math::Random& random) const {
    if (!m_block) return false;
    return state.is(m_block) && random.nextFloat() < m_probability;
}

std::unique_ptr<RuleTest> RandomBlockMatchRuleTest::clone() const {
    return std::make_unique<RandomBlockMatchRuleTest>(m_block, m_probability);
}

// ============================================================================
// RandomBlockStateMatchRuleTest 实现
// ============================================================================

RandomBlockStateMatchRuleTest::RandomBlockStateMatchRuleTest(const BlockState* state, f32 probability)
    : m_state(state)
    , m_probability(probability) {}

bool RandomBlockStateMatchRuleTest::test(const BlockState& state, math::Random& random) const {
    if (!m_state) return false;
    return state.stateId() == m_state->stateId() && random.nextFloat() < m_probability;
}

std::unique_ptr<RuleTest> RandomBlockStateMatchRuleTest::clone() const {
    return std::make_unique<RandomBlockStateMatchRuleTest>(m_state, m_probability);
}

// ============================================================================
// TagMatchRuleTest 实现
// ============================================================================

TagMatchRuleTest::TagMatchRuleTest(const std::string& tagName)
    : m_tagName(tagName) {}

bool TagMatchRuleTest::test(const BlockState& state, math::Random& random) const {
    (void)random;
    // TODO: 实现标签系统支持
    // 需要通过 BlockTags 系统检查方块是否属于指定标签
    // 当前返回 false，等待标签系统实现后更新
    (void)state;
    return false;
}

std::unique_ptr<RuleTest> TagMatchRuleTest::clone() const {
    return std::make_unique<TagMatchRuleTest>(m_tagName);
}

// ============================================================================
// StoneRuleTest 实现
// ============================================================================

bool StoneRuleTest::test(const BlockState& state, math::Random& random) const {
    (void)random;
    // 匹配石头、花岗岩、闪长岩、安山岩
    return state.is(VanillaBlocks::STONE) ||
           state.is(VanillaBlocks::GRANITE) ||
           state.is(VanillaBlocks::DIORITE) ||
           state.is(VanillaBlocks::ANDESITE);
}

std::unique_ptr<RuleTest> StoneRuleTest::clone() const {
    return std::make_unique<StoneRuleTest>();
}

// ============================================================================
// SimpleBlockStateProvider 实现
// ============================================================================

SimpleBlockStateProvider::SimpleBlockStateProvider(const BlockState* state)
    : m_state(state) {}

const BlockState* SimpleBlockStateProvider::getState(math::Random& random, i32 x, i32 y, i32 z) const {
    (void)random;
    (void)x; (void)y; (void)z;
    return m_state;
}

// ============================================================================
// OreFeatureConfig 实现
// ============================================================================

OreFeatureConfig::OreFeatureConfig(std::unique_ptr<RuleTest> targetRule, const BlockState* oreState, i32 veinSize)
    : target(std::move(targetRule))
    , state(oreState)
    , size(veinSize) {}

std::unique_ptr<RuleTest> OreFeatureConfig::naturalStone() {
    return std::make_unique<StoneRuleTest>();
}

// ============================================================================
// createOreTarget 实现
// ============================================================================

std::unique_ptr<RuleTest> createOreTarget(OreTargetType type) {
    switch (type) {
        case OreTargetType::NaturalStone:
            return std::make_unique<StoneRuleTest>();
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
