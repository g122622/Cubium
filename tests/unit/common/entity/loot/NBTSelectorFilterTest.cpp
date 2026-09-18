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
 * The above copyright notice shall be included in all
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

/**
 * @file NBTSelectorFilterTest.cpp
 * @brief 实体选择器 NBT 过滤逻辑的单元测试
 *
 * 测试 EntitySelector 的 nbt= 参数所依赖的核心组件：
 * - NBTPredicate::matchNBT 公开静态方法的子集匹配语义
 * - NBTPredicate::matchTag 公开静态方法的标签比较语义
 * - 与 EntitySelector::NbtCondition 协作的匹配+取反逻辑
 */

#include <gtest/gtest.h>

#include "common/advancement/trigger/conditions/NBTPredicate.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/util/nbt/Nbt.hpp"

using namespace mc;
using namespace mc::advancement;
using namespace mc::command;
using namespace mc::nbt;
using namespace mc::nbt::tags;

// ============================================================================
// NBTPredicate::matchNBT 公开静态方法测试
// ============================================================================

class NBTMatchTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(NBTMatchTest, ExactMatchReturnsTrue)
{
    compound_tag expected;
    expected.value.emplace("name", std::make_unique<string_tag>("Steve"));

    compound_tag actual;
    actual.value.emplace("name", std::make_unique<string_tag>("Steve"));

    EXPECT_TRUE(NBTPredicate::matchNBT(expected, actual));
}

TEST_F(NBTMatchTest, SubsetMatchReturnsTrue)
{
    // 期望是实际的子集
    compound_tag expected;
    expected.value.emplace("name", std::make_unique<string_tag>("Steve"));

    compound_tag actual;
    actual.value.emplace("name", std::make_unique<string_tag>("Steve"));
    actual.value.emplace("health", std::make_unique<float_tag>(20.0f));

    EXPECT_TRUE(NBTPredicate::matchNBT(expected, actual));
}

TEST_F(NBTMatchTest, MissingKeyReturnsFalse)
{
    compound_tag expected;
    expected.value.emplace("name", std::make_unique<string_tag>("Steve"));
    expected.value.emplace("level", std::make_unique<int_tag>(10));

    compound_tag actual;
    actual.value.emplace("name", std::make_unique<string_tag>("Steve"));

    EXPECT_FALSE(NBTPredicate::matchNBT(expected, actual));
}

TEST_F(NBTMatchTest, ValueMismatchReturnsFalse)
{
    compound_tag expected;
    expected.value.emplace("name", std::make_unique<string_tag>("Steve"));

    compound_tag actual;
    actual.value.emplace("name", std::make_unique<string_tag>("Alex"));

    EXPECT_FALSE(NBTPredicate::matchNBT(expected, actual));
}

TEST_F(NBTMatchTest, EmptyExpectedMatchesAnyCompound)
{
    compound_tag expected; // 空的复合标签

    compound_tag actual;
    actual.value.emplace("name", std::make_unique<string_tag>("Steve"));

    EXPECT_TRUE(NBTPredicate::matchNBT(expected, actual));
}

TEST_F(NBTMatchTest, NestedCompoundMatch)
{
    compound_tag expected;
    auto innerExpected = std::make_unique<compound_tag>();
    innerExpected->value.emplace("x", std::make_unique<int_tag>(1));
    expected.value.emplace("pos", std::move(innerExpected));

    compound_tag actual;
    auto innerActual = std::make_unique<compound_tag>();
    innerActual->value.emplace("x", std::make_unique<int_tag>(1));
    innerActual->value.emplace("y", std::make_unique<int_tag>(2));
    actual.value.emplace("pos", std::move(innerActual));

    EXPECT_TRUE(NBTPredicate::matchNBT(expected, actual));
}

TEST_F(NBTMatchTest, ListSubsetMatch)
{
    // 列表子集匹配：期望列表中的每个元素必须在实际列表中存在匹配
    compound_tag expected;
    auto expList = std::make_unique<string_list_tag>();
    expList->value.push_back("tag1");
    expected.value.emplace("Tags", std::move(expList));

    compound_tag actual;
    auto actList = std::make_unique<string_list_tag>();
    actList->value.push_back("tag1");
    actList->value.push_back("tag2");
    actual.value.emplace("Tags", std::move(actList));

    EXPECT_TRUE(NBTPredicate::matchNBT(expected, actual));
}

TEST_F(NBTMatchTest, ListMissingItemReturnsFalse)
{
    compound_tag expected;
    auto expList = std::make_unique<string_list_tag>();
    expList->value.push_back("tag1");
    expList->value.push_back("tag3");
    expected.value.emplace("Tags", std::move(expList));

    compound_tag actual;
    auto actList = std::make_unique<string_list_tag>();
    actList->value.push_back("tag1");
    actList->value.push_back("tag2");
    actual.value.emplace("Tags", std::move(actList));

    EXPECT_FALSE(NBTPredicate::matchNBT(expected, actual));
}

TEST_F(NBTMatchTest, DifferentTagTypesReturnFalse)
{
    compound_tag expected;
    expected.value.emplace("value", std::make_unique<int_tag>(42));

    compound_tag actual;
    actual.value.emplace("value", std::make_unique<string_tag>("42"));

    EXPECT_FALSE(NBTPredicate::matchNBT(expected, actual));
}

TEST_F(NBTMatchTest, NumericTypeMismatchReturnsFalse)
{
    compound_tag expected;
    expected.value.emplace("value", std::make_unique<int_tag>(42));

    compound_tag actual;
    actual.value.emplace("value", std::make_unique<long_tag>(42));

    EXPECT_FALSE(NBTPredicate::matchNBT(expected, actual));
}

// ============================================================================
// NBTPredicate::matchTag 公开静态方法测试
// ============================================================================

class NBTMatchTagTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(NBTMatchTagTest, ByteTagMatch)
{
    byte_tag expected(1);
    byte_tag actual(1);
    EXPECT_TRUE(NBTPredicate::matchTag(expected, actual));
}

TEST_F(NBTMatchTagTest, ByteTagMismatch)
{
    byte_tag expected(1);
    byte_tag actual(2);
    EXPECT_FALSE(NBTPredicate::matchTag(expected, actual));
}

TEST_F(NBTMatchTagTest, IntTagMatch)
{
    int_tag expected(42);
    int_tag actual(42);
    EXPECT_TRUE(NBTPredicate::matchTag(expected, actual));
}

TEST_F(NBTMatchTagTest, StringTagMatch)
{
    string_tag expected("hello");
    string_tag actual("hello");
    EXPECT_TRUE(NBTPredicate::matchTag(expected, actual));
}

TEST_F(NBTMatchTagTest, FloatTagApproximateMatch)
{
    float_tag expected(3.14f);
    float_tag actual(3.14f);
    EXPECT_TRUE(NBTPredicate::matchTag(expected, actual));
}

TEST_F(NBTMatchTagTest, EndTagMatchesAnything)
{
    end_tag expected;
    end_tag actual;
    EXPECT_TRUE(NBTPredicate::matchTag(expected, actual));
}

TEST_F(NBTMatchTagTest, DifferentTypesReturnFalse)
{
    int_tag expected(1);
    byte_tag actual(1);
    EXPECT_FALSE(NBTPredicate::matchTag(expected, actual));
}

// ============================================================================
// EntitySelector::NbtCondition 与 matchNBT 协作测试
// ============================================================================

class NbtConditionFilterTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(NbtConditionFilterTest, NbtConditionHoldsCompoundTag)
{
    // 验证 NbtCondition 能正确持有和访问 compound_tag
    EntitySelector::NbtCondition condition;
    EXPECT_FALSE(condition.hasCondition());

    auto tag = std::make_unique<compound_tag>();
    tag->value.emplace("CustomName", std::make_unique<string_tag>("TestName"));
    condition.nbt = std::shared_ptr<compound_tag>(tag.release());
    condition.negated = false;

    EXPECT_TRUE(condition.hasCondition());
    ASSERT_NE(condition.nbt.get(), nullptr);
    EXPECT_FALSE(condition.negated);
}

TEST_F(NbtConditionFilterTest, NbtConditionNegated)
{
    EntitySelector::NbtCondition condition;
    auto tag = std::make_unique<compound_tag>();
    tag->value.emplace("OnGround", std::make_unique<byte_tag>(1));
    condition.nbt = std::shared_ptr<compound_tag>(tag.release());
    condition.negated = true;

    EXPECT_TRUE(condition.hasCondition());
    EXPECT_TRUE(condition.negated);
}

TEST_F(NbtConditionFilterTest, MatchWithNegatedLogic)
{
    // 模拟 EntityResolver 中的 NBT 过滤逻辑：
    // matches = matchNBT(query, entity) 然后 if (negated) matches = !matches

    compound_tag query;
    query.value.emplace("Health", std::make_unique<float_tag>(20.0f));

    compound_tag entityNbt;
    entityNbt.value.emplace("Health", std::make_unique<float_tag>(20.0f));
    entityNbt.value.emplace("Pos", std::make_unique<string_tag>("dummy"));

    // 不取反：匹配成功
    bool matches = NBTPredicate::matchNBT(query, entityNbt);
    EXPECT_TRUE(matches);

    // 取反：结果翻转为 false
    bool negated = true;
    if (negated) {
        matches = !matches;
    }
    EXPECT_FALSE(matches);
}

TEST_F(NbtConditionFilterTest, NoMatchWithNegatedLogic)
{
    compound_tag query;
    query.value.emplace("Health", std::make_unique<float_tag>(10.0f));

    compound_tag entityNbt;
    entityNbt.value.emplace("Health", std::make_unique<float_tag>(20.0f));

    // 不取反：匹配失败
    bool matches = NBTPredicate::matchNBT(query, entityNbt);
    EXPECT_FALSE(matches);

    // 取反：结果翻转为 true（即 nbt=!{Health:10.0f} 对于 Health=20.0f 应该匹配）
    bool negated = true;
    if (negated) {
        matches = !matches;
    }
    EXPECT_TRUE(matches);
}

TEST_F(NbtConditionFilterTest, NullQueryTagReturnsFalse)
{
    EntitySelector::NbtCondition condition;
    // nbt 为 nullptr，hasCondition() 返回 false
    EXPECT_FALSE(condition.hasCondition());

    compound_tag entityNbt;
    entityNbt.value.emplace("Health", std::make_unique<float_tag>(20.0f));

    // 当 queryTag 为 nullptr 时，matchNBT 不应被调用
    // 模拟 EntityResolver 中的逻辑：(queryTag != nullptr) && matchNBT(...)
    const auto* queryTag = condition.nbt.get();
    bool matches = (queryTag != nullptr) && NBTPredicate::matchNBT(*queryTag, entityNbt);
    EXPECT_FALSE(matches);
}

TEST_F(NbtConditionFilterTest, ComplexNbtQuery)
{
    // 测试复杂 NBT 查询（嵌套复合标签 + 列表）
    compound_tag query;
    query.value.emplace("CustomName", std::make_unique<string_tag>("Diamond Sword"));
    auto queryEnch = std::make_unique<compound_tag>();
    queryEnch->value.emplace("id", std::make_unique<string_tag>("minecraft:sharpness"));
    query.value.emplace("Enchantment", std::move(queryEnch));

    compound_tag entityNbt;
    entityNbt.value.emplace("CustomName", std::make_unique<string_tag>("Diamond Sword"));
    entityNbt.value.emplace("Damage", std::make_unique<int_tag>(100));
    auto actualEnch = std::make_unique<compound_tag>();
    actualEnch->value.emplace("id", std::make_unique<string_tag>("minecraft:sharpness"));
    actualEnch->value.emplace("lvl", std::make_unique<short_tag>(5));
    entityNbt.value.emplace("Enchantment", std::move(actualEnch));

    EXPECT_TRUE(NBTPredicate::matchNBT(query, entityNbt));
}

TEST_F(NbtConditionFilterTest, SelectedItemFieldInPlayerNbt)
{
    // 模拟玩家实体 NBT 中 SelectedItem 字段的子集匹配
    compound_tag query;
    auto querySelectedItem = std::make_unique<compound_tag>();
    querySelectedItem->value.emplace("id", std::make_unique<string_tag>("minecraft:diamond_sword"));
    query.value.emplace("SelectedItem", std::move(querySelectedItem));

    compound_tag entityNbt;
    entityNbt.value.emplace("Pos", std::make_unique<string_tag>("dummy"));
    auto actualSelected = std::make_unique<compound_tag>();
    actualSelected->value.emplace("id", std::make_unique<string_tag>("minecraft:diamond_sword"));
    actualSelected->value.emplace("Count", std::make_unique<byte_tag>(1));
    actualSelected->value.emplace("Damage", std::make_unique<int_tag>(0));
    entityNbt.value.emplace("SelectedItem", std::move(actualSelected));

    EXPECT_TRUE(NBTPredicate::matchNBT(query, entityNbt));
}

// ============================================================================
// LootParameterSets::selector() 参数集测试
// ============================================================================

#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/context/LootParams.hpp"

class SelectorParameterSetTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SelectorParameterSetTest, SelectorType)
{
    auto paramSet = loot::LootParameterSets::selector();
    EXPECT_EQ(paramSet.getType(), loot::LootParameterSet::Type::Selector);
}

TEST_F(SelectorParameterSetTest, SelectorName)
{
    auto paramSet = loot::LootParameterSets::selector();
    EXPECT_EQ(paramSet.getName(), "minecraft:selector");
}

TEST_F(SelectorParameterSetTest, SelectorContainsThisEntity)
{
    auto paramSet = loot::LootParameterSets::selector();
    EXPECT_TRUE(paramSet.contains(loot::LootParams::THIS_ENTITY.getId()));
}

TEST_F(SelectorParameterSetTest, SelectorContainsBlockPos)
{
    auto paramSet = loot::LootParameterSets::selector();
    EXPECT_TRUE(paramSet.contains(loot::LootParams::BLOCK_POS.getId()));
}

TEST_F(SelectorParameterSetTest, SelectorDoesNotContainKillerPlayer)
{
    auto paramSet = loot::LootParameterSets::selector();
    EXPECT_FALSE(paramSet.contains(loot::LootParams::KILLER_PLAYER.getId()));
}

TEST_F(SelectorParameterSetTest, SelectorDoesNotContainDamageSource)
{
    auto paramSet = loot::LootParameterSets::selector();
    EXPECT_FALSE(paramSet.contains(loot::LootParams::DAMAGE_SOURCE.getId()));
}

TEST_F(SelectorParameterSetTest, SelectorValidateWithRequiredParams)
{
    auto paramSet = loot::LootParameterSets::selector();
    // 提供必需参数
    std::vector<std::string> provided = {
        loot::LootParams::THIS_ENTITY.getId(),
        loot::LootParams::BLOCK_POS.getId(),
    };
    EXPECT_TRUE(paramSet.validate(provided));
}

TEST_F(SelectorParameterSetTest, SelectorValidateMissingRequiredParam)
{
    auto paramSet = loot::LootParameterSets::selector();
    // 缺少 BLOCK_POS
    std::vector<std::string> provided = {
        loot::LootParams::THIS_ENTITY.getId(),
    };
    EXPECT_FALSE(paramSet.validate(provided));
}

// ============================================================================
// EntitySelector::PredicateCondition 测试
// ============================================================================

#include "common/resource/ResourceLocation.hpp"

class PredicateConditionTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(PredicateConditionTest, DefaultConditionHasMinecraftNamespace)
{
    // 默认构造的 ResourceLocation 有 "minecraft" 命名空间，所以 isValid() 返回 true。
    // 但 path() 为空，表示未引用任何具体谓词，因此 hasCondition() 返回 false
    // （与 NbtCondition 默认 nullptr 时 hasCondition()==false 的语义一致，
    //  参见 src/server/command/README.md 的说明）。
    EntitySelector::PredicateCondition condition;
    EXPECT_EQ(condition.predicate.namespace_(), "minecraft");
    EXPECT_TRUE(condition.predicate.isValid());
    EXPECT_TRUE(condition.predicate.path().empty());
    EXPECT_FALSE(condition.hasCondition());
}

TEST_F(PredicateConditionTest, ValidPredicateHasCondition)
{
    EntitySelector::PredicateCondition condition;
    condition.predicate = ResourceLocation::parse("minecraft:example_predicate");
    condition.negated = false;
    EXPECT_TRUE(condition.hasCondition());
}

TEST_F(PredicateConditionTest, PredicateNegated)
{
    EntitySelector::PredicateCondition condition;
    condition.predicate = ResourceLocation::parse("minecraft:example_predicate");
    condition.negated = true;
    EXPECT_TRUE(condition.hasCondition());
    EXPECT_TRUE(condition.negated);
}

TEST_F(PredicateConditionTest, PredicateIdToString)
{
    EntitySelector::PredicateCondition condition;
    condition.predicate = ResourceLocation::parse("minecraft:gameplay/raid");
    EXPECT_EQ(condition.predicate.toString(), "minecraft:gameplay/raid");
}

// ============================================================================
// 谓词过滤集成测试（LootPredicateManager + LootContext + ReferenceCondition）
// ============================================================================

#include "common/TestWorldHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/loot/LootPredicateManager.hpp"
#include "common/item/loot/conditions/RandomChanceCondition.hpp"
#include "common/item/loot/conditions/ReferenceCondition.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/tick/manager/TickManager.hpp"

using namespace mc::loot;
using namespace mc::world;

class PredicateFilterTestWorld : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= MIN_BUILD_HEIGHT && y < MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] tick::TickManager& tickManager() override
    {
        throw std::runtime_error("PredicateFilterTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("PredicateFilterTestWorld::tickManager not implemented");
    }
};

class PredicateFilterTest : public ::testing::Test {
protected:
    PredicateFilterTestWorld m_world;
    LootPredicateManager m_predicateManager;
    math::Random m_random{12345};

    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    std::unique_ptr<LootContext> createContext()
    {
        return LootContextBuilder(m_world)
            .withRandom(m_random)
            .withPredicateResolver(
                [this](const std::string& id) -> const LootCondition* { return m_predicateManager.getPredicate(id); })
            .build(loot::LootParameterSets::selector());
    }
};

TEST_F(PredicateFilterTest, PredicateFoundTrue_EntitySelectorMatch)
{
    // 注册一个总是返回 true 的谓词
    m_predicateManager.registerPredicate("minecraft:always_true", std::make_unique<RandomChanceCondition>(1.0f));

    auto context = createContext();
    const auto* condition = m_predicateManager.getPredicate("minecraft:always_true");
    ASSERT_NE(condition, nullptr);

    // 模拟 EntityResolver 中的谓词评估逻辑
    bool matches = false;
    if (!context->pushPredicate(condition)) {
        matches = false; // 循环引用检测
    } else {
        matches = condition->test(*context);
        context->popPredicate(condition);
    }
    EXPECT_TRUE(matches);

    // 不取反
    bool negated = false;
    if (negated) {
        matches = !matches;
    }
    EXPECT_TRUE(matches);
}

TEST_F(PredicateFilterTest, PredicateFoundTrue_Negated)
{
    m_predicateManager.registerPredicate("minecraft:always_true", std::make_unique<RandomChanceCondition>(1.0f));

    auto context = createContext();
    const auto* condition = m_predicateManager.getPredicate("minecraft:always_true");
    ASSERT_NE(condition, nullptr);

    bool matches = false;
    if (!context->pushPredicate(condition)) {
        matches = false;
    } else {
        matches = condition->test(*context);
        context->popPredicate(condition);
    }
    EXPECT_TRUE(matches);

    // 取反：predicate=!minecraft:always_true
    bool negated = true;
    if (negated) {
        matches = !matches;
    }
    EXPECT_FALSE(matches);
}

TEST_F(PredicateFilterTest, PredicateFoundFalse_EntitySelectorNoMatch)
{
    // 注册一个总是返回 false 的谓词
    m_predicateManager.registerPredicate("minecraft:always_false", std::make_unique<RandomChanceCondition>(0.0f));

    auto context = createContext();
    const auto* condition = m_predicateManager.getPredicate("minecraft:always_false");
    ASSERT_NE(condition, nullptr);

    bool matches = false;
    if (!context->pushPredicate(condition)) {
        matches = false;
    } else {
        matches = condition->test(*context);
        context->popPredicate(condition);
    }
    EXPECT_FALSE(matches);
}

TEST_F(PredicateFilterTest, PredicateFoundFalse_Negated)
{
    m_predicateManager.registerPredicate("minecraft:always_false", std::make_unique<RandomChanceCondition>(0.0f));

    auto context = createContext();
    const auto* condition = m_predicateManager.getPredicate("minecraft:always_false");
    ASSERT_NE(condition, nullptr);

    bool matches = false;
    if (!context->pushPredicate(condition)) {
        matches = false;
    } else {
        matches = condition->test(*context);
        context->popPredicate(condition);
    }
    EXPECT_FALSE(matches);

    // 取反：predicate=!minecraft:always_false → true
    bool negated = true;
    if (negated) {
        matches = !matches;
    }
    EXPECT_TRUE(matches);
}

TEST_F(PredicateFilterTest, PredicateNotFound_ReturnsFalse)
{
    // 不注册任何谓词
    auto context = createContext();
    const auto* condition = m_predicateManager.getPredicate("minecraft:nonexistent");

    // 模拟 EntityResolver 中的逻辑：谓词不存在返回 false
    EXPECT_EQ(condition, nullptr);
    bool matches = false; // condition 为 nullptr 时不执行 test
    // 取反：predicate=!minecraft:nonexistent → true
    bool negated = true;
    if (negated) {
        matches = !matches;
    }
    EXPECT_TRUE(matches);
}

TEST_F(PredicateFilterTest, CircularReference_ReturnsFalse)
{
    // 注册一个引用自身的谓词（循环引用）
    auto refCondition = std::make_unique<ReferenceCondition>("minecraft:circular");
    m_predicateManager.registerPredicate("minecraft:circular", std::move(refCondition));

    auto context = createContext();
    const auto* condition = m_predicateManager.getPredicate("minecraft:circular");
    ASSERT_NE(condition, nullptr);

    // 循环引用检测应返回 false
    bool matches = false;
    if (!context->pushPredicate(condition)) {
        matches = false; // 检测到循环引用
    } else {
        matches = condition->test(*context);
        context->popPredicate(condition);
    }
    EXPECT_FALSE(matches);
}

TEST_F(PredicateFilterTest, SelectorParameterSetInContext)
{
    // 验证使用 LootParameterSets::selector() 构建的 LootContext 可以正常工作
    auto context = createContext();
    ASSERT_NE(context, nullptr);
    EXPECT_FALSE(context->has(loot::LootParams::KILLER_PLAYER));
    EXPECT_FALSE(context->has(loot::LootParams::DAMAGE_SOURCE));
}
