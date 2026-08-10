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
#include "item/core/ItemRegistry.hpp"
#include "item/loot/LootPool.hpp"
#include "item/loot/LootTable.hpp"
#include "item/loot/StatePropertiesPredicate.hpp"
#include "item/loot/conditions/LootConditions.hpp"
#include "item/loot/context/LootContext.hpp"
#include "item/loot/entries/ItemLootEntry.hpp"
#include "item/loot/entries/LootEntry.hpp"
#include "item/loot/entries/LootEntryBuilder.hpp"
#include "item/loot/functions/ApplyBonusFunction.hpp"
#include "util/math/random/Random.hpp"
#include "util/math/random/RandomRanges.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/chunk/data/ChunkData.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/tick/manager/TickManager.hpp"

using namespace mc;
using namespace mc::loot;

// Test implementation of IWorld for loot testing
class LootConditionTestWorld : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("LootConditionTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("LootConditionTestWorld::tickManager not implemented");
    }
};

class LootConditionsTest : public ::testing::Test {
protected:
    LootConditionTestWorld m_world;

    void SetUp() override
    {
        // 初始化方块和物品系统
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(LootConditionsTest, RandomChanceCondition_Basic)
{
    math::Random random(12345);

    // 100% 概率
    RandomChanceCondition alwaysTrue(1.0f);
    EXPECT_TRUE(alwaysTrue.test(*LootContextBuilder(m_world).withRandom(random).build()));

    // 0% 概率
    RandomChanceCondition neverTrue(0.0f);
    EXPECT_FALSE(neverTrue.test(*LootContextBuilder(m_world).withRandom(random).build()));

    // 50% 概率 - 统计测试
    RandomChanceCondition fiftyFifty(0.5f);
    i32 trueCount = 0;
    for (i32 i = 0; i < 1000; ++i) {
        random.setSeed(i);
        if (fiftyFifty.test(*LootContextBuilder(m_world).withRandom(random).build())) {
            ++trueCount;
        }
    }
    // 应该接近 50% (450-550)
    EXPECT_GT(trueCount, 400);
    EXPECT_LT(trueCount, 600);
}

TEST_F(LootConditionsTest, NotCondition_Basic)
{
    math::Random random(12345);

    RandomChanceCondition alwaysTrue(1.0f);
    NotCondition notAlwaysTrue(std::make_unique<RandomChanceCondition>(1.0f));

    EXPECT_TRUE(alwaysTrue.test(*LootContextBuilder(m_world).withRandom(random).build()));
    EXPECT_FALSE(notAlwaysTrue.test(*LootContextBuilder(m_world).withRandom(random).build()));

    RandomChanceCondition neverTrue(0.0f);
    NotCondition notNeverTrue(std::make_unique<RandomChanceCondition>(0.0f));

    EXPECT_FALSE(neverTrue.test(*LootContextBuilder(m_world).withRandom(random).build()));
    EXPECT_TRUE(notNeverTrue.test(*LootContextBuilder(m_world).withRandom(random).build()));
}

TEST_F(LootConditionsTest, AndCondition_AllTrue)
{
    math::Random random(12345);

    auto condition = std::make_unique<AndCondition>();
    condition->addCondition(std::make_unique<RandomChanceCondition>(1.0f));
    condition->addCondition(std::make_unique<RandomChanceCondition>(1.0f));
    condition->addCondition(std::make_unique<RandomChanceCondition>(1.0f));

    EXPECT_TRUE(condition->test(*LootContextBuilder(m_world).withRandom(random).build()));
}

TEST_F(LootConditionsTest, AndCondition_OneFalse)
{
    math::Random random(12345);

    auto condition = std::make_unique<AndCondition>();
    condition->addCondition(std::make_unique<RandomChanceCondition>(1.0f));
    condition->addCondition(std::make_unique<RandomChanceCondition>(0.0f)); // 这个为 false
    condition->addCondition(std::make_unique<RandomChanceCondition>(1.0f));

    EXPECT_FALSE(condition->test(*LootContextBuilder(m_world).withRandom(random).build()));
}

TEST_F(LootConditionsTest, OrCondition_AllFalse)
{
    math::Random random(12345);

    auto condition = std::make_unique<OrCondition>();
    condition->addCondition(std::make_unique<RandomChanceCondition>(0.0f));
    condition->addCondition(std::make_unique<RandomChanceCondition>(0.0f));
    condition->addCondition(std::make_unique<RandomChanceCondition>(0.0f));

    EXPECT_FALSE(condition->test(*LootContextBuilder(m_world).withRandom(random).build()));
}

TEST_F(LootConditionsTest, OrCondition_OneTrue)
{
    math::Random random(12345);

    auto condition = std::make_unique<OrCondition>();
    condition->addCondition(std::make_unique<RandomChanceCondition>(0.0f));
    condition->addCondition(std::make_unique<RandomChanceCondition>(1.0f)); // 这个为 true
    condition->addCondition(std::make_unique<RandomChanceCondition>(0.0f));

    EXPECT_TRUE(condition->test(*LootContextBuilder(m_world).withRandom(random).build()));
}

TEST_F(LootConditionsTest, FortuneCondition_GetLevel)
{
    // FortuneCondition::getFortuneLevel 从 LootContext 的 FORTUNE_LEVEL 参数获取
    math::Random random(12345);

    auto context = LootContextBuilder(m_world).withRandom(random).build();

    // 使用 setOwnedValue 设置时运等级
    context->setOwnedValue(LootParams::FORTUNE_LEVEL, 3);

    EXPECT_EQ(FortuneCondition::getFortuneLevel(*context), 3);

    // 测试默认值（未设置时返回 0）
    auto context2 = LootContextBuilder(m_world).withRandom(random).build();

    EXPECT_EQ(FortuneCondition::getFortuneLevel(*context2), 0);
}

TEST_F(LootConditionsTest, FortuneCondition_ApplyBonus)
{
    math::Random random(12345);

    // Fortune 0 - 无加成
    EXPECT_EQ(ApplyBonusFunction::calculateOreDrops(1, 0, random), 1);

    // Fortune I - 大约 33% 概率 +1
    i32 total = 0;
    for (i32 i = 0; i < 1000; ++i) {
        random.setSeed(i);
        total += ApplyBonusFunction::calculateOreDrops(1, 1, random);
    }
    // 平均应该在 1.33 左右，总和约 1330
    EXPECT_GT(total, 1200);
    EXPECT_LT(total, 1500);

    // Fortune III - 最多 +3
    random.setSeed(12345);
    for (i32 i = 0; i < 1000; ++i) {
        i32 result = ApplyBonusFunction::calculateOreDrops(1, 3, random);
        EXPECT_GE(result, 1);
        EXPECT_LE(result, 4); // 1 + 3 = 4
    }
}

// ============================================================================
// LootConditionBuilder 测试
// ============================================================================

TEST(LootConditionBuilderTest, FactoryMethods)
{
    auto silkTouch = LootConditionBuilder::silkTouch();
    EXPECT_NE(silkTouch, nullptr);
    EXPECT_EQ(silkTouch->getType(), "silk_touch");

    auto fortune = LootConditionBuilder::fortune(2);
    EXPECT_NE(fortune, nullptr);
    EXPECT_EQ(fortune->getType(), "fortune");

    auto randomChance = LootConditionBuilder::randomChance(0.5f);
    EXPECT_NE(randomChance, nullptr);
    EXPECT_EQ(randomChance->getType(), "random_chance");

    auto notCondition = LootConditionBuilder::not_(LootConditionBuilder::silkTouch());
    EXPECT_NE(notCondition, nullptr);
    EXPECT_EQ(notCondition->getType(), "inverted");

    // and_ 和 or_ 测试需要单独测试，因为 unique_ptr 不能放入初始化列表
    std::vector<std::unique_ptr<LootCondition>> andConditions;
    andConditions.push_back(LootConditionBuilder::randomChance(0.5f));
    andConditions.push_back(LootConditionBuilder::fortune(1));
    // and_() 返回 AndCondition，其 type 为 "and"（OrCondition 为 "or"）。
    auto andCondition = LootConditionBuilder::and_(std::move(andConditions));
    EXPECT_NE(andCondition, nullptr);
    EXPECT_EQ(andCondition->getType(), "and");

    std::vector<std::unique_ptr<LootCondition>> orConditions;
    orConditions.push_back(LootConditionBuilder::silkTouch());
    orConditions.push_back(LootConditionBuilder::fortune(3));
    auto orCondition = LootConditionBuilder::or_(std::move(orConditions));
    EXPECT_NE(orCondition, nullptr);
    EXPECT_EQ(orCondition->getType(), "or");

    auto tableBonus = LootConditionBuilder::tableBonus("minecraft:fortune", {0.1f, 0.15f, 0.2f, 0.25f});
    EXPECT_NE(tableBonus, nullptr);
    EXPECT_EQ(tableBonus->getType(), "table_bonus");
}

// ============================================================================
// LootEntry 条件测试
// ============================================================================

class LootEntryConditionTest : public ::testing::Test {
protected:
    LootConditionTestWorld m_world;

    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(LootEntryConditionTest, EntryWithCondition)
{
    math::Random random(12345);

    // 创建一个带条件的物品条目
    ItemLootEntry entry("minecraft:diamond", RandomValueRange(1.0f, 1.0f), 1, 0);
    entry.addCondition(std::make_unique<RandomChanceCondition>(0.0f)); // 永远不满足

    // 创建 LootContext
    auto context = LootContextBuilder(m_world).withRandom(random).build();

    // 测试条件
    EXPECT_FALSE(entry.testConditions(*context));
}

TEST_F(LootEntryConditionTest, EntryWithMultipleConditions)
{
    math::Random random(12345);

    ItemLootEntry entry("minecraft:diamond", RandomValueRange(1.0f, 1.0f), 1, 0);
    entry.addCondition(std::make_unique<RandomChanceCondition>(1.0f));
    entry.addCondition(std::make_unique<RandomChanceCondition>(1.0f));
    entry.addCondition(std::make_unique<RandomChanceCondition>(1.0f));

    auto context = LootContextBuilder(m_world).withRandom(random).build();
    EXPECT_TRUE(entry.testConditions(*context));

    // 添加一个失败的条件
    entry.addCondition(std::make_unique<RandomChanceCondition>(0.0f));
    EXPECT_FALSE(entry.testConditions(*context));
}

TEST_F(LootEntryConditionTest, EntryConditionCloning)
{
    ItemLootEntry original("minecraft:diamond", RandomValueRange(1.0f, 1.0f), 1, 0);
    original.addCondition(std::make_unique<RandomChanceCondition>(0.5f));

    auto cloned = original.clone();
    EXPECT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->getType(), LootEntryType::Item);

    // 验证克隆后的条件数量
    const auto& conditions = cloned->getConditions();
    EXPECT_EQ(conditions.size(), 1);
}

// ============================================================================
// LootPool 条件测试
// ============================================================================

TEST_F(LootEntryConditionTest, PoolWithEntryCondition)
{
    // LootPool 本身不支持条件，但可以通过在 Entry 上添加条件实现相同效果
    math::Random random(12345);

    auto pool = std::make_unique<LootPool>(RandomValueRange(1.0f, 1.0f));

    // 创建带条件的条目（永远不会满足）
    auto entry = std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f, 1.0f), 1, 0);
    entry->addCondition(std::make_unique<RandomChanceCondition>(0.0f));
    pool->addEntry(std::move(entry));

    LootTable table;
    table.addPool(std::move(pool));

    auto context = LootContextBuilder(m_world).withRandom(random).build();
    auto drops = table.generate(*context);

    // 条件不满足，不应该生成掉落
    EXPECT_TRUE(drops.empty());
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST_F(LootConditionsTest, ClonePreservesState)
{
    FortuneCondition original(2);
    auto cloned = original.clone();

    EXPECT_EQ(cloned->getType(), "fortune");

    auto* fortuneClone = dynamic_cast<FortuneCondition*>(cloned.get());
    ASSERT_NE(fortuneClone, nullptr);
}

TEST_F(LootConditionsTest, NotConditionWithNullCondition)
{
    // NotCondition 应该安全处理 null 内部条件
    NotCondition notNull(nullptr);
    math::Random random(12345);
    auto context = LootContextBuilder(m_world).withRandom(random).build();

    // null 条件被视为 true，取反后为 false
    EXPECT_TRUE(notNull.test(*context));
}

TEST_F(LootConditionsTest, EmptyAndCondition)
{
    // 空 AndCondition 应该返回 true（所有 0 个条件都满足）
    AndCondition emptyAnd;
    math::Random random(12345);
    auto context = LootContextBuilder(m_world).withRandom(random).build();

    EXPECT_TRUE(emptyAnd.test(*context));
}

TEST_F(LootConditionsTest, EmptyOrCondition)
{
    // 空 OrCondition 应该返回 false（没有条件满足）
    OrCondition emptyOr;
    math::Random random(12345);
    auto context = LootContextBuilder(m_world).withRandom(random).build();

    EXPECT_FALSE(emptyOr.test(*context));
}

// ============================================================================
// StatePropertiesPredicate 测试
// ============================================================================

class StatePropertiesPredicateTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(StatePropertiesPredicateTest, EmptyPredicate)
{
    // 空谓词应该匹配任何方块状态
    StatePropertiesPredicate empty;
    EXPECT_TRUE(empty.isEmpty());
    EXPECT_EQ(empty.matcherCount(), 0);

    // 获取一个真实的方块状态进行测试
    const BlockState& state = VanillaBlocks::STONE->defaultState();
    EXPECT_TRUE(empty.matches(state));
}

TEST_F(StatePropertiesPredicateTest, ExactMatcher_BooleanProperty)
{
    // 测试布尔属性的精确匹配
    StatePropertiesPredicate predicate;
    predicate.addExactMatch("lit", "true");

    // 获取红石灯（有 lit 属性）并测试
    const auto& lampStates = VanillaBlocks::REDSTONE_LAMP->stateContainer().validStates();
    int litCount = 0;
    int unlitCount = 0;

    for (size_t i = 0; i < lampStates.size(); ++i) {
        const BlockState* state = lampStates[i].get();
        if (predicate.matches(*state)) {
            litCount++;
        } else {
            unlitCount++;
        }
    }

    // 红石灯有 lit 和 unlit 两个状态
    EXPECT_EQ(litCount, 1);
    EXPECT_EQ(unlitCount, 1);
}

TEST_F(StatePropertiesPredicateTest, ExactMatcher_IntegerProperty)
{
    // 测试整数属性的精确匹配 - 使用红石灯
    StatePropertiesPredicate predicate;
    predicate.addExactMatch("lit", "false");

    // 获取红石灯状态
    const BlockState& defaultState = VanillaBlocks::REDSTONE_LAMP->defaultState();

    // 默认状态应该是 unlit
    EXPECT_TRUE(predicate.matches(defaultState));
}

TEST_F(StatePropertiesPredicateTest, MultipleMatchers)
{
    // 测试多属性匹配 - 使用门
    StatePropertiesPredicate predicate;
    predicate.addExactMatch("facing", "north");
    predicate.addExactMatch("open", "true");

    // 获取门（有 facing 和 open 属性）
    const auto& doorStates = VanillaBlocks::OAK_DOOR->stateContainer().validStates();
    int matchCount = 0;
    for (size_t i = 0; i < doorStates.size(); ++i) {
        const BlockState* state = doorStates[i].get();
        if (predicate.matches(*state)) {
            matchCount++;
        }
    }
    // 应该至少有一个状态匹配（facing=north, open=true）
    EXPECT_GT(matchCount, 0);
}

TEST_F(StatePropertiesPredicateTest, Clone)
{
    StatePropertiesPredicate original;
    original.addExactMatch("age", "3");
    original.addRangeMatch("power", "5", "10");

    StatePropertiesPredicate cloned = original;

    EXPECT_EQ(original.matcherCount(), cloned.matcherCount());
    EXPECT_EQ(original.matcherCount(), 2);
}

TEST_F(StatePropertiesPredicateTest, ToJson)
{
    StatePropertiesPredicate predicate;
    predicate.addExactMatch("age", "3");

    std::string json = predicate.toJson();
    EXPECT_TRUE(json.find("age") != std::string::npos);
    EXPECT_TRUE(json.find("3") != std::string::npos);

    predicate.addRangeMatch("power", "5", "10");
    json = predicate.toJson();
    EXPECT_TRUE(json.find("power") != std::string::npos);
    EXPECT_TRUE(json.find("min") != std::string::npos);
    EXPECT_TRUE(json.find("max") != std::string::npos);
}

// ============================================================================
// BlockStateCondition 测试
// ============================================================================

class BlockStateConditionTest : public ::testing::Test {
protected:
    LootConditionTestWorld m_world;

    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(BlockStateConditionTest, BlockIdOnly)
{
    math::Random random(12345);

    // 创建一个只检查方块 ID 的条件
    BlockStateCondition condition("minecraft:stone");

    // 测试不匹配的方块
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    auto dirtContext = LootContextBuilder(m_world)
                           .withRandom(random)
                           .withParameter(LootParams::BLOCK_STATE, const_cast<BlockState*>(&dirtState))
                           .build();

    EXPECT_FALSE(condition.test(*dirtContext));

    // 测试匹配的方块
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto stoneContext = LootContextBuilder(m_world)
                            .withRandom(random)
                            .withParameter(LootParams::BLOCK_STATE, const_cast<BlockState*>(&stoneState))
                            .build();

    EXPECT_TRUE(condition.test(*stoneContext));
}

TEST_F(BlockStateConditionTest, NoBlockState)
{
    math::Random random(12345);

    BlockStateCondition condition("minecraft:stone");

    // 没有设置 BLOCK_STATE 参数
    auto context = LootContextBuilder(m_world).withRandom(random).build();
    EXPECT_FALSE(condition.test(*context));
}

TEST_F(BlockStateConditionTest, WithProperties_ExactMatch)
{
    math::Random random(12345);

    // 创建带属性匹配的条件 - 使用红石灯的 lit 属性
    StatePropertiesPredicate properties;
    properties.addExactMatch("lit", "true");

    BlockStateCondition condition("minecraft:redstone_lamp", std::move(properties));

    // 获取红石灯状态
    const auto& lampStates = VanillaBlocks::REDSTONE_LAMP->stateContainer().validStates();

    int matchCount = 0;
    for (size_t i = 0; i < lampStates.size(); ++i) {
        const BlockState* state = lampStates[i].get();
        auto context = LootContextBuilder(m_world)
                           .withRandom(random)
                           .withParameter(LootParams::BLOCK_STATE, const_cast<BlockState*>(state))
                           .build();

        if (condition.test(*context)) {
            matchCount++;
        }
    }

    // 只有一个状态匹配（lit=true）
    EXPECT_EQ(matchCount, 1);
}

TEST_F(BlockStateConditionTest, WithProperties_WrongBlock)
{
    math::Random random(12345);

    StatePropertiesPredicate properties;
    properties.addExactMatch("lit", "true");

    BlockStateCondition condition("minecraft:redstone_lamp", std::move(properties));

    // 用石头测试（没有 lit 属性）
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    auto context = LootContextBuilder(m_world)
                       .withRandom(random)
                       .withParameter(LootParams::BLOCK_STATE, const_cast<BlockState*>(&stoneState))
                       .build();

    // 方块 ID 不匹配
    EXPECT_FALSE(condition.test(*context));
}

TEST_F(BlockStateConditionTest, Clone)
{
    StatePropertiesPredicate properties;
    properties.addExactMatch("lit", "true");

    BlockStateCondition original("minecraft:redstone_lamp", std::move(properties));
    auto cloned = original.clone();

    auto* blockStateCond = dynamic_cast<BlockStateCondition*>(cloned.get());
    ASSERT_NE(blockStateCond, nullptr);
    EXPECT_EQ(blockStateCond->getBlockId(), "minecraft:redstone_lamp");
    EXPECT_EQ(blockStateCond->getProperties().matcherCount(), 1);
}

TEST_F(BlockStateConditionTest, BuilderMethods)
{
    // 测试只有方块 ID 的构建
    auto condition1 = LootConditionBuilder::blockState("minecraft:stone");
    EXPECT_NE(condition1, nullptr);
    EXPECT_EQ(condition1->getType(), "block_state_property");

    auto* blockStateCond1 = dynamic_cast<BlockStateCondition*>(condition1.get());
    ASSERT_NE(blockStateCond1, nullptr);
    EXPECT_EQ(blockStateCond1->getBlockId(), "minecraft:stone");
    EXPECT_TRUE(blockStateCond1->getProperties().isEmpty());

    // 测试带属性的构建
    StatePropertiesPredicate properties;
    properties.addExactMatch("lit", "true");

    auto condition2 = LootConditionBuilder::blockState("minecraft:redstone_lamp", std::move(properties));
    EXPECT_NE(condition2, nullptr);

    auto* blockStateCond2 = dynamic_cast<BlockStateCondition*>(condition2.get());
    ASSERT_NE(blockStateCond2, nullptr);
    EXPECT_EQ(blockStateCond2->getBlockId(), "minecraft:redstone_lamp");
    EXPECT_EQ(blockStateCond2->getProperties().matcherCount(), 1);
}

// ============================================================================
// FishingOpenWaterCondition 测试
// ============================================================================

class FishingOpenWaterConditionTest : public ::testing::Test {
protected:
    LootConditionTestWorld m_world;

    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(FishingOpenWaterConditionTest, RequiresOpenWater)
{
    math::Random random(12345);

    // 创建需要开放水域的条件
    FishingOpenWaterCondition condition(true);

    // 在开放水域中应该返回 true
    auto openWaterContext =
        LootContextBuilder(m_world).withRandom(random).withOwnedValue(LootParams::IS_IN_OPEN_WATER, true).build();
    EXPECT_TRUE(condition.test(*openWaterContext));

    // 不在开放水域中应该返回 false
    auto closedWaterContext =
        LootContextBuilder(m_world).withRandom(random).withOwnedValue(LootParams::IS_IN_OPEN_WATER, false).build();
    EXPECT_FALSE(condition.test(*closedWaterContext));
}

TEST_F(FishingOpenWaterConditionTest, DoesNotRequireOpenWater)
{
    math::Random random(12345);

    // 创建不需要开放水域的条件
    FishingOpenWaterCondition condition(false);

    // 在开放水域中应该返回 false
    auto openWaterContext =
        LootContextBuilder(m_world).withRandom(random).withOwnedValue(LootParams::IS_IN_OPEN_WATER, true).build();
    EXPECT_FALSE(condition.test(*openWaterContext));

    // 不在开放水域中应该返回 true
    auto closedWaterContext =
        LootContextBuilder(m_world).withRandom(random).withOwnedValue(LootParams::IS_IN_OPEN_WATER, false).build();
    EXPECT_TRUE(condition.test(*closedWaterContext));
}

TEST_F(FishingOpenWaterConditionTest, MissingParameter)
{
    math::Random random(12345);

    // 当参数未设置时，需要开放水域的条件应该返回 false
    FishingOpenWaterCondition requireOpenWater(true);
    auto contextWithoutParam = LootContextBuilder(m_world).withRandom(random).build();
    EXPECT_FALSE(requireOpenWater.test(*contextWithoutParam));

    // 当参数未设置时，不需要开放水域的条件应该返回 true
    FishingOpenWaterCondition notRequireOpenWater(false);
    EXPECT_TRUE(notRequireOpenWater.test(*contextWithoutParam));
}

TEST_F(FishingOpenWaterConditionTest, Clone)
{
    FishingOpenWaterCondition original(true);
    auto cloned = original.clone();

    EXPECT_EQ(cloned->getType(), "fishing_hook_in_open_water");

    auto* fishingCond = dynamic_cast<FishingOpenWaterCondition*>(cloned.get());
    ASSERT_NE(fishingCond, nullptr);
    EXPECT_TRUE(fishingCond->requireOpenWater());
}

TEST_F(FishingOpenWaterConditionTest, TypeIdentifier)
{
    FishingOpenWaterCondition condition(true);
    EXPECT_EQ(condition.getType(), "fishing_hook_in_open_water");
}
