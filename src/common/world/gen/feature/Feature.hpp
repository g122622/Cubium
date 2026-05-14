#pragma once

#include "../../../core/Types.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../block/Block.hpp"
#include "../../chunk/ChunkPos.hpp"
#include <memory>
#include <vector>

namespace mc {

// 前向声明
class BlockState;
class ChunkPrimer;
class Biome;
class WorldGenRegion;
class IChunkGenerator;

// ============================================================================
// RuleTest - 方块匹配规则基类
// ============================================================================

/**
 * @brief 方块匹配规则基类
 *
 * 用于结构模板中的规则测试，判断方块是否满足特定条件。
 * 参考 MC 1.16.5: RuleTest
 *
 * 子类实现：
 * - AlwaysTrueRuleTest: 总是返回 true
 * - BlockMatchRuleTest: 匹配特定方块
 * - BlockStateMatchRuleTest: 匹配特定方块状态
 * - RandomBlockMatchRuleTest: 带概率的方块匹配
 * - RandomBlockStateMatchRuleTest: 带概率的方块状态匹配
 * - TagMatchRuleTest: 匹配方块标签
 */
class RuleTest {
public:
    virtual ~RuleTest() = default;

    /**
     * @brief 测试方块状态是否匹配规则
     * @param state 方块状态
     * @param random 随机数生成器
     * @return 是否匹配
     */
    [[nodiscard]] virtual bool test(const BlockState& state, math::Random& random) const = 0;

    /**
     * @brief 获取规则类型名称
     */
    [[nodiscard]] virtual const char* name() const = 0;

    /**
     * @brief 克隆规则
     */
    [[nodiscard]] virtual std::unique_ptr<RuleTest> clone() const = 0;
};

// ============================================================================
// AlwaysTrueRuleTest - 总是为真的规则
// ============================================================================

/**
 * @brief 总是返回 true 的规则测试
 *
 * 参考 MC 1.16.5: AlwaysTrueRuleTest
 */
class AlwaysTrueRuleTest : public RuleTest {
public:
    static const AlwaysTrueRuleTest INSTANCE;

    [[nodiscard]] bool test(const BlockState& state, math::Random& random) const override;
    [[nodiscard]] const char* name() const override { return "always_true"; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override;

private:
    AlwaysTrueRuleTest() = default;
};

// ============================================================================
// BlockMatchRuleTest - 方块匹配规则
// ============================================================================

/**
 * @brief 匹配特定方块的规则测试
 *
 * 只检查方块类型，不检查方块状态属性。
 * 参考 MC 1.16.5: BlockMatchRuleTest
 */
class BlockMatchRuleTest : public RuleTest {
public:
    explicit BlockMatchRuleTest(const Block* block);

    [[nodiscard]] bool test(const BlockState& state, math::Random& random) const override;
    [[nodiscard]] const char* name() const override { return "block_match"; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override;

    [[nodiscard]] const Block* getBlock() const { return m_block; }

private:
    const Block* m_block;
};

// ============================================================================
// BlockStateMatchRuleTest - 方块状态匹配规则
// ============================================================================

/**
 * @brief 匹配特定方块状态的规则测试
 *
 * 完全匹配方块状态（包括所有属性）。
 * 参考 MC 1.16.5: BlockStateMatchRuleTest
 */
class BlockStateMatchRuleTest : public RuleTest {
public:
    explicit BlockStateMatchRuleTest(const BlockState* state);

    [[nodiscard]] bool test(const BlockState& state, math::Random& random) const override;
    [[nodiscard]] const char* name() const override { return "block_state_match"; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override;

    [[nodiscard]] const BlockState* getBlockState() const { return m_state; }

private:
    const BlockState* m_state;
};

// ============================================================================
// RandomBlockMatchRuleTest - 随机方块匹配规则
// ============================================================================

/**
 * @brief 带概率的方块匹配规则测试
 *
 * 当方块匹配且有随机概率命中时返回 true。
 * 参考 MC 1.16.5: RandomBlockMatchRuleTest
 */
class RandomBlockMatchRuleTest : public RuleTest {
public:
    RandomBlockMatchRuleTest(const Block* block, f32 probability);

    [[nodiscard]] bool test(const BlockState& state, math::Random& random) const override;
    [[nodiscard]] const char* name() const override { return "random_block_match"; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override;

    [[nodiscard]] const Block* getBlock() const { return m_block; }
    [[nodiscard]] f32 getProbability() const { return m_probability; }

private:
    const Block* m_block;
    f32 m_probability;
};

// ============================================================================
// RandomBlockStateMatchRuleTest - 随机方块状态匹配规则
// ============================================================================

/**
 * @brief 带概率的方块状态匹配规则测试
 *
 * 当方块状态完全匹配且有随机概率命中时返回 true。
 * 参考 MC 1.16.5: RandomBlockStateMatchRuleTest
 */
class RandomBlockStateMatchRuleTest : public RuleTest {
public:
    RandomBlockStateMatchRuleTest(const BlockState* state, f32 probability);

    [[nodiscard]] bool test(const BlockState& state, math::Random& random) const override;
    [[nodiscard]] const char* name() const override { return "random_block_state_match"; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override;

    [[nodiscard]] const BlockState* getBlockState() const { return m_state; }
    [[nodiscard]] f32 getProbability() const { return m_probability; }

private:
    const BlockState* m_state;
    f32 m_probability;
};

// ============================================================================
// TagMatchRuleTest - 标签匹配规则
// ============================================================================

/**
 * @brief 匹配方块标签的规则测试
 *
 * 当方块属于指定标签时返回 true。
 * 参考 MC 1.16.5: TagMatchRuleTest
 *
 * 注意：需要标签系统支持
 */
class TagMatchRuleTest : public RuleTest {
public:
    explicit TagMatchRuleTest(const std::string& tagName);

    [[nodiscard]] bool test(const BlockState& state, math::Random& random) const override;
    [[nodiscard]] const char* name() const override { return "tag_match"; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override;

    [[nodiscard]] const std::string& getTagName() const { return m_tagName; }

private:
    std::string m_tagName;
    // TODO: 添加标签引用，需要标签系统支持
};

// ============================================================================
// StoneRuleTest - 石头规则测试（内部使用）
// ============================================================================

/**
 * @brief 匹配石头类方块的规则测试
 *
 * 匹配石头、花岗岩、闪长岩、安山岩。
 * 用于自然矿石生成。
 */
class StoneRuleTest : public RuleTest {
public:
    [[nodiscard]] bool test(const BlockState& state, math::Random& random) const override;
    [[nodiscard]] const char* name() const override { return "stone"; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override;
};

// ============================================================================
// BlockStateProvider - 方块状态提供者
// ============================================================================

/**
 * @brief 方块状态提供者
 *
 * 用于提供方块状态，可以是固定的或基于噪声的。
 */
class BlockStateProvider {
public:
    virtual ~BlockStateProvider() = default;

    /**
     * @brief 获取方块状态
     * @param random 随机数生成器
     * @param pos 位置
     * @return 方块状态
     */
    [[nodiscard]] virtual const BlockState* getState(math::Random& random, i32 x, i32 y, i32 z) const = 0;
};

/**
 * @brief 固定方块状态提供者
 */
class SimpleBlockStateProvider : public BlockStateProvider {
public:
    explicit SimpleBlockStateProvider(const BlockState* state);

    [[nodiscard]] const BlockState* getState(math::Random& random, i32 x, i32 y, i32 z) const override;

private:
    const BlockState* m_state;
};

// ============================================================================
// OreFeatureConfig - 矿石特征配置
// ============================================================================

/**
 * @brief 特征配置基类
 *
 * 所有特征配置的基类接口。
 */
struct IFeatureConfig {
    virtual ~IFeatureConfig() = default;
};

/**
 * @brief 矿石特征配置
 *
 * 参考 MC OreFeatureConfig，定义矿石生成的参数。
 */
struct OreFeatureConfig : public IFeatureConfig {
    /// 目标方块规则（哪些方块可被替换为矿石）
    std::unique_ptr<RuleTest> target;

    /// 矿石方块状态
    const BlockState* state = nullptr;

    /// 矿脉大小（方块数量）
    i32 size;

    /**
     * @brief 构造矿石配置
     * @param targetRule 目标方块规则
     * @param oreState 矿石方块状态
     * @param veinSize 矿脉大小
     */
    OreFeatureConfig(std::unique_ptr<RuleTest> targetRule, const BlockState* oreState, i32 veinSize);

    /**
     * @brief 创建自然石头目标配置（用于主世界矿石）
     */
    static std::unique_ptr<RuleTest> naturalStone();
};

/**
 * @brief 矿石目标类型枚举
 *
 * 定义常见的矿石生成目标。
 */
enum class OreTargetType {
    NaturalStone, ///< 石头、花岗岩、闪长岩、安山岩
    Netherrack,   ///< 下界岩
    Basalt        ///< 玄武岩
};

/**
 * @brief 创建目标规则
 * @param type 目标类型
 * @return 目标规则
 */
std::unique_ptr<RuleTest> createOreTarget(OreTargetType type);

} // namespace mc
