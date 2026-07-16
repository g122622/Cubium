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

#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include <memory>
#include <vector>

namespace mc {

// 前向声明
class BlockState;
namespace world::chunk {
class ChunkPrimer;
}
using world::chunk::ChunkPrimer;
class WorldGenRegion;
class IChunkGenerator;

// ============================================================================
// RuleTest - 方块匹配规则基类
// ============================================================================

/**
 * @brief 方块匹配规则基类
 *
 * 用于结构模板中的规则测试，判断方块是否满足特定条件。
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
 * 提供单例 INSTANCE 便于共享，同时允许直接构造（结构处理器等消费者需要
 * 独立拥有所有权时使用 std::make_unique<AlwaysTrueRuleTest>()）。
 */
class AlwaysTrueRuleTest : public RuleTest {
public:
    static const AlwaysTrueRuleTest INSTANCE;

    AlwaysTrueRuleTest() = default;
    [[nodiscard]] bool test(const BlockState& state, math::Random& random) const override;
    [[nodiscard]] const char* name() const override { return "always_true"; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override;
};

// ============================================================================
// BlockMatchRuleTest - 方块匹配规则
// ============================================================================

/**
 * @brief 匹配特定方块的规则测试
 *
 * 只检查方块类型，不检查方块状态属性。
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
 * 当方块属于指定标签时返回 true。标签以 ResourceLocation 标识。
 */
class TagMatchRuleTest : public RuleTest {
public:
    explicit TagMatchRuleTest(const ResourceLocation& tagId);

    [[nodiscard]] bool test(const BlockState& state, math::Random& random) const override;
    [[nodiscard]] const char* name() const override { return "tag_match"; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override;

    [[nodiscard]] const ResourceLocation& getTagName() const { return m_tagId; }

private:
    ResourceLocation m_tagId;
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

/**
 * @brief 匹配深板岩类方块的规则测试
 *
 * 匹配深板岩和凝灰岩（MC 1.21: DEEPSLATE_ORE_REPLACEABLES 标签）。
 * 用于深层矿石生成，使深层矿石变体在深板岩区域正确替换。
 */
class DeepslateRuleTest : public RuleTest {
public:
    [[nodiscard]] bool test(const BlockState& state, math::Random& random) const override;
    [[nodiscard]] const char* name() const override { return "deepslate"; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override;
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
 * @brief 矿石目标（目标规则 + 对应矿石方块）
 *
 * MC 1.21: 对应 OreConfiguration.TargetBlockState。
 * 每个目标定义了"匹配哪些方块 → 放置什么矿石"。
 * 例如：石头区域放铁矿，深板岩区域放深层铁矿。
 */
struct OreTarget {
    /// 目标方块规则（哪些方块可被替换）
    std::unique_ptr<RuleTest> target;

    /// 匹配目标时放置的矿石方块状态
    const BlockState* state = nullptr;

    OreTarget(std::unique_ptr<RuleTest> targetRule, const BlockState* oreState)
        : target(std::move(targetRule))
        , state(oreState)
    {}
};

/**
 * @brief 矿石特征配置
 *
 * 定义矿石生成的参数。
 * 参考 MC 1.21.11: OreConfiguration
 *
 * 支持多目标列表：遍历 targets，使用第一个匹配的目标放置对应矿石。
 * 例如：铁矿配置 targets[0]=(石头→铁矿), targets[1]=(深板岩→深层铁矿)。
 * 当矿石生成在石头中时匹配 targets[0] 放置铁矿，
 * 生成在深板岩中时匹配 targets[1] 放置深层铁矿。
 */
struct OreFeatureConfig : public IFeatureConfig {
    /// 多目标列表（MC 1.21: targetStates）
    std::vector<OreTarget> targets;

    /// 矿脉大小（方块数量）
    i32 size;

    /**
     * @brief 空气暴露丢弃概率
     *
     * 当矿石方块相邻有空气时，以此概率跳过放置。
     * 0.0 = 不丢弃（默认），1.0 = 总是丢弃暴露在空气中的矿石。
     * 参考 MC 1.21.11: OreConfiguration.discardChanceOnAirExposure
     */
    f32 discardChanceOnAirExposure = 0.0f;

    /**
     * @brief 构造矿石配置（多目标）
     * @param oreTargets 目标列表
     * @param veinSize 矿脉大小
     * @param discardChance 空气暴露丢弃概率（默认 0.0）
     */
    OreFeatureConfig(std::vector<OreTarget> oreTargets, i32 veinSize, f32 discardChance = 0.0f);

    /**
     * @brief 构造矿石配置（单目标，向后兼容）
     * @param targetRule 目标方块规则
     * @param oreState 矿石方块状态
     * @param veinSize 矿脉大小
     * @param discardChance 空气暴露丢弃概率（默认 0.0）
     */
    OreFeatureConfig(
        std::unique_ptr<RuleTest> targetRule, const BlockState* oreState, i32 veinSize, f32 discardChance = 0.0f);

    /**
     * @brief 创建自然石头目标配置（用于主世界矿石）
     */
    static std::unique_ptr<RuleTest> naturalStone();

    /**
     * @brief 创建深板岩目标配置（用于主世界深层矿石）
     */
    static std::unique_ptr<RuleTest> deepslateStone();

    /**
     * @brief 创建同时匹配石头和深板岩的矿石目标列表
     * @param stoneOre 石头区域的矿石方块
     * @param deepslateOre 深板岩区域的矿石方块
     */
    static std::vector<OreTarget> stoneAndDeepslateOre(const BlockState* stoneOre, const BlockState* deepslateOre);
};

/**
 * @brief 矿石目标类型枚举
 *
 * 定义常见的矿石生成目标。
 */
enum class OreTargetType {
    NaturalStone, ///< 石头、花岗岩、闪长岩、安山岩
    Deepslate,    ///< 深板岩、凝灰岩
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
