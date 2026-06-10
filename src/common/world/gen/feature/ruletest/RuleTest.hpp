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
#include "resource/ResourceLocation.hpp"
#include <memory>

namespace mc {

class BlockState;

namespace world {
namespace gen {
namespace feature {
namespace ruletest {

// BlockState 定义在 mc 命名空间，通过 using 引入 ruletest 命名空间以便使用
using ::mc::BlockState;

// ============================================================================
// RuleTest - 方块匹配规则基类
// ============================================================================

/**
 * @brief 方块匹配规则基类
 *
 * 用于判断方块是否满足特定条件。
 * 矿石生成和结构模板处理器均使用此体系。
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
// AlwaysTrueRuleTest
// ============================================================================

/**
 * @brief 总是返回 true 的规则测试
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
// BlockMatchRuleTest
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
// BlockStateMatchRuleTest
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
// RandomBlockMatchRuleTest
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
// RandomBlockStateMatchRuleTest
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
// TagMatchRuleTest
// ============================================================================

/**
 * @brief 匹配方块标签的规则测试
 *
 * 当方块属于指定标签时返回 true。
 */
class TagMatchRuleTest : public RuleTest {
public:
    explicit TagMatchRuleTest(const std::string& tagName);
    explicit TagMatchRuleTest(const ResourceLocation& tagId);

    [[nodiscard]] bool test(const BlockState& state, math::Random& random) const override;
    [[nodiscard]] const char* name() const override { return "tag_match"; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override;

    [[nodiscard]] const std::string& getTagName() const { return m_tagName; }

private:
    std::string m_tagName;
};

} // namespace ruletest
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
