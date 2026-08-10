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

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Constants.hpp"
#include "item/Items.hpp"
#include "item/loot/LootPredicateManager.hpp"
#include "item/loot/conditions/LootConditions.hpp"
#include "item/loot/conditions/ReferenceCondition.hpp"
#include "item/loot/context/LootContext.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "world/tick/manager/TickManager.hpp"

using namespace mc;
using namespace mc::loot;

// 测试用 IWorld 实现
class ReferenceConditionTestWorld : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ReferenceConditionTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ReferenceConditionTestWorld::tickManager not implemented");
    }
};

// ============================================================================
// ReferenceCondition 基础测试
// ============================================================================

class ReferenceConditionTest : public ::testing::Test {
protected:
    ReferenceConditionTestWorld m_world;
    LootPredicateManager m_predicateManager;
    math::Random m_random{12345};

    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    /**
     * @brief 创建一个配置了谓词解析器的 LootContext
     */
    std::unique_ptr<LootContext> createContext()
    {
        return LootContextBuilder(m_world)
            .withRandom(m_random)
            .withPredicateResolver(
                [this](const std::string& id) -> const LootCondition* { return m_predicateManager.getPredicate(id); })
            .build();
    }
};

TEST_F(ReferenceConditionTest, GetType)
{
    ReferenceCondition condition("minecraft:test/predicate");
    EXPECT_EQ(condition.getType(), "reference");
}

TEST_F(ReferenceConditionTest, GetName)
{
    ReferenceCondition condition("minecraft:gameplay/raid");
    EXPECT_EQ(condition.getName(), "minecraft:gameplay/raid");
}

TEST_F(ReferenceConditionTest, Clone)
{
    ReferenceCondition original("minecraft:test/clone_test");
    auto cloned = original.clone();

    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->getType(), "reference");

    auto* refClone = dynamic_cast<ReferenceCondition*>(cloned.get());
    ASSERT_NE(refClone, nullptr);
    EXPECT_EQ(refClone->getName(), "minecraft:test/clone_test");
}

// ============================================================================
// ReferenceCondition 谓词查找测试
// ============================================================================

TEST_F(ReferenceConditionTest, PredicateFound_ReturnsTrue)
{
    // 注册一个总是返回 true 的谓词
    m_predicateManager.registerPredicate("minecraft:always_true", std::make_unique<RandomChanceCondition>(1.0f));

    ReferenceCondition condition("minecraft:always_true");
    auto context = createContext();

    EXPECT_TRUE(condition.test(*context));
}

TEST_F(ReferenceConditionTest, PredicateFound_ReturnsFalse)
{
    // 注册一个总是返回 false 的谓词
    m_predicateManager.registerPredicate("minecraft:always_false", std::make_unique<RandomChanceCondition>(0.0f));

    ReferenceCondition condition("minecraft:always_false");
    auto context = createContext();

    EXPECT_FALSE(condition.test(*context));
}

TEST_F(ReferenceConditionTest, PredicateNotFound_ReturnsFalse)
{
    // 不注册任何谓词，引用不存在的谓词
    ReferenceCondition condition("minecraft:nonexistent_predicate");
    auto context = createContext();

    // MC 参考实现：ConditionReference.test() 在找不到谓词时返回 false
    EXPECT_FALSE(condition.test(*context));
}

TEST_F(ReferenceConditionTest, NoPredicateResolver_ReturnsFalse)
{
    // 不设置谓词解析器
    ReferenceCondition condition("minecraft:test/no_resolver");
    auto context = LootContextBuilder(m_world).withRandom(m_random).build();

    // 没有解析器，应该返回 false
    EXPECT_FALSE(condition.test(*context));
}

// ============================================================================
// ReferenceCondition 循环引用检测测试
// ============================================================================

/**
 * @brief 自引用谓词条件
 *
 * 用于测试循环引用检测。此条件引用自身，形成无限循环。
 */
class SelfReferencingCondition : public LootCondition {
public:
    explicit SelfReferencingCondition(const std::string& selfId)
        : m_selfId(selfId)
    {}

    [[nodiscard]] bool test(LootContext& context) const override
    {
        // 创建一个 ReferenceCondition 引用自身
        ReferenceCondition ref(m_selfId);
        return ref.test(context);
    }

    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override
    {
        return std::make_unique<SelfReferencingCondition>(m_selfId);
    }

    [[nodiscard]] std::string getType() const override { return "self_referencing"; }

private:
    std::string m_selfId;
};

TEST_F(ReferenceConditionTest, CircularReference_DetectsAndReturnsFalse)
{
    // 注册一个自引用谓词
    m_predicateManager.registerPredicate(
        "minecraft:self_ref", std::make_unique<SelfReferencingCondition>("minecraft:self_ref"));

    ReferenceCondition condition("minecraft:self_ref");
    auto context = createContext();

    // 循环引用应被检测到，返回 false 而不是无限递归
    EXPECT_FALSE(condition.test(*context));
}

/**
 * @brief 互相引用的条件 A
 *
 * A 引用 B，B 引用 A，形成循环。
 */
class MutualRefConditionA : public LootCondition {
public:
    [[nodiscard]] bool test(LootContext& context) const override
    {
        ReferenceCondition ref("minecraft:mutual_b");
        return ref.test(context);
    }

    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override
    {
        return std::make_unique<MutualRefConditionA>();
    }

    [[nodiscard]] std::string getType() const override { return "mutual_ref_a"; }
};

/**
 * @brief 互相引用的条件 B
 *
 * B 引用 A，A 引用 B，形成循环。
 */
class MutualRefConditionB : public LootCondition {
public:
    [[nodiscard]] bool test(LootContext& context) const override
    {
        ReferenceCondition ref("minecraft:mutual_a");
        return ref.test(context);
    }

    [[nodiscard]] std::unique_ptr<LootCondition> clone() const noexcept override
    {
        return std::make_unique<MutualRefConditionB>();
    }

    [[nodiscard]] std::string getType() const override { return "mutual_ref_b"; }
};

TEST_F(ReferenceConditionTest, MutualCircularReference_DetectsAndReturnsFalse)
{
    // 注册互相引用的条件
    m_predicateManager.registerPredicate("minecraft:mutual_a", std::make_unique<MutualRefConditionA>());
    m_predicateManager.registerPredicate("minecraft:mutual_b", std::make_unique<MutualRefConditionB>());

    ReferenceCondition condition("minecraft:mutual_a");
    auto context = createContext();

    // 互相引用的循环应被检测到，返回 false 而不是无限递归
    EXPECT_FALSE(condition.test(*context));
}

// ============================================================================
// ReferenceCondition 多级引用测试
// ============================================================================

TEST_F(ReferenceConditionTest, ChainedReference_Works)
{
    // A -> B -> C(returns true)
    m_predicateManager.registerPredicate("minecraft:chain_c", std::make_unique<RandomChanceCondition>(1.0f));
    m_predicateManager.registerPredicate(
        "minecraft:chain_b", std::make_unique<ReferenceCondition>("minecraft:chain_c"));
    m_predicateManager.registerPredicate(
        "minecraft:chain_a", std::make_unique<ReferenceCondition>("minecraft:chain_b"));

    ReferenceCondition condition("minecraft:chain_a");
    auto context = createContext();

    // 链式引用最终到达返回 true 的条件
    EXPECT_TRUE(condition.test(*context));
}

TEST_F(ReferenceConditionTest, ChainedReference_MiddleFalse)
{
    // A -> B(returns false) -> C
    m_predicateManager.registerPredicate("minecraft:chain_c", std::make_unique<RandomChanceCondition>(1.0f));
    m_predicateManager.registerPredicate("minecraft:chain_b", std::make_unique<RandomChanceCondition>(0.0f));
    m_predicateManager.registerPredicate(
        "minecraft:chain_a", std::make_unique<ReferenceCondition>("minecraft:chain_b"));

    ReferenceCondition condition("minecraft:chain_a");
    auto context = createContext();

    // 中间条件返回 false，链式引用结果为 false
    EXPECT_FALSE(condition.test(*context));
}

// ============================================================================
// ReferenceCondition 与 AndCondition/OrCondition 组合测试
// ============================================================================

TEST_F(ReferenceConditionTest, ReferenceInAndCondition)
{
    // 将 ReferenceCondition 嵌入 AndCondition
    m_predicateManager.registerPredicate("minecraft:ref_true", std::make_unique<RandomChanceCondition>(1.0f));

    auto andCondition = std::make_unique<AndCondition>();
    andCondition->addCondition(std::make_unique<ReferenceCondition>("minecraft:ref_true"));
    andCondition->addCondition(std::make_unique<RandomChanceCondition>(1.0f));

    auto context = createContext();
    EXPECT_TRUE(andCondition->test(*context));
}

TEST_F(ReferenceConditionTest, ReferenceInOrCondition)
{
    // 将 ReferenceCondition 嵌入 OrCondition
    m_predicateManager.registerPredicate("minecraft:ref_true", std::make_unique<RandomChanceCondition>(1.0f));

    auto orCondition = std::make_unique<OrCondition>();
    orCondition->addCondition(std::make_unique<RandomChanceCondition>(0.0f));
    orCondition->addCondition(std::make_unique<ReferenceCondition>("minecraft:ref_true"));

    auto context = createContext();
    EXPECT_TRUE(orCondition->test(*context));
}

TEST_F(ReferenceConditionTest, ReferenceInNotCondition)
{
    // 将 ReferenceCondition 嵌入 NotCondition
    m_predicateManager.registerPredicate("minecraft:ref_true", std::make_unique<RandomChanceCondition>(1.0f));

    NotCondition notCondition(std::make_unique<ReferenceCondition>("minecraft:ref_true"));

    auto context = createContext();
    // Not(true) = false
    EXPECT_FALSE(notCondition.test(*context));
}

// ============================================================================
// LootContext 循环引用检测测试（直接测试 pushPredicate/popPredicate）
// ============================================================================

class LootContextPredicateStackTest : public ::testing::Test {
protected:
    ReferenceConditionTestWorld m_world;
    math::Random m_random{12345};

    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(LootContextPredicateStackTest, PushPredicate_FirstTimeReturnsTrue)
{
    auto context = LootContextBuilder(m_world).withRandom(m_random).build();
    LootCondition* dummy = reinterpret_cast<LootCondition*>(0x1);

    EXPECT_TRUE(context->pushPredicate(dummy));
    context->popPredicate(dummy);
}

TEST_F(LootContextPredicateStackTest, PushPredicate_SamePointerReturnsFalse)
{
    auto context = LootContextBuilder(m_world).withRandom(m_random).build();
    LootCondition* dummy = reinterpret_cast<LootCondition*>(0x1);

    EXPECT_TRUE(context->pushPredicate(dummy));
    EXPECT_FALSE(context->pushPredicate(dummy)); // 再次 push 同一个指针应返回 false

    context->popPredicate(dummy);
}

TEST_F(LootContextPredicateStackTest, PushPredicate_DifferentPointersReturnTrue)
{
    auto context = LootContextBuilder(m_world).withRandom(m_random).build();
    LootCondition* dummy1 = reinterpret_cast<LootCondition*>(0x1);
    LootCondition* dummy2 = reinterpret_cast<LootCondition*>(0x2);

    EXPECT_TRUE(context->pushPredicate(dummy1));
    EXPECT_TRUE(context->pushPredicate(dummy2)); // 不同指针可以正常 push

    context->popPredicate(dummy2);
    context->popPredicate(dummy1);
}

TEST_F(LootContextPredicateStackTest, PopPredicate_AllowsRePush)
{
    auto context = LootContextBuilder(m_world).withRandom(m_random).build();
    LootCondition* dummy = reinterpret_cast<LootCondition*>(0x1);

    EXPECT_TRUE(context->pushPredicate(dummy));
    context->popPredicate(dummy);
    // pop 后可以再次 push
    EXPECT_TRUE(context->pushPredicate(dummy));
    context->popPredicate(dummy);
}

TEST_F(LootContextPredicateStackTest, GetPredicate_WithResolver)
{
    LootPredicateManager manager;
    manager.registerPredicate("minecraft:test", std::make_unique<RandomChanceCondition>(0.5f));

    auto context = LootContextBuilder(m_world)
                       .withRandom(m_random)
                       .withPredicateResolver([&manager](const std::string& id) -> const LootCondition* {
                           return manager.getPredicate(id);
                       })
                       .build();

    const LootCondition* result = context->getPredicate("minecraft:test");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->getType(), "random_chance");
}

TEST_F(LootContextPredicateStackTest, GetPredicate_WithoutResolver)
{
    auto context = LootContextBuilder(m_world).withRandom(m_random).build();

    const LootCondition* result = context->getPredicate("minecraft:test");
    EXPECT_EQ(result, nullptr);
}

TEST_F(LootContextPredicateStackTest, GetPredicate_NonExistent)
{
    LootPredicateManager manager;

    auto context = LootContextBuilder(m_world)
                       .withRandom(m_random)
                       .withPredicateResolver([&manager](const std::string& id) -> const LootCondition* {
                           return manager.getPredicate(id);
                       })
                       .build();

    const LootCondition* result = context->getPredicate("minecraft:does_not_exist");
    EXPECT_EQ(result, nullptr);
}
