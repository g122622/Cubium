#include <gtest/gtest.h>

#include "entity/loot/LootContext.hpp"
#include "entity/loot/LootTable.hpp"
#include "entity/loot/LootPool.hpp"
#include "entity/loot/LootEntry.hpp"
#include "entity/loot/LootConditions.hpp"
#include "entity/loot/RandomRanges.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/Items.hpp"
#include "world/IWorld.hpp"
#include "world/chunk/ChunkData.hpp"
#include "world/block/Block.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/fluid/Fluid.hpp"
#include "util/math/random/Random.hpp"

using namespace mc;
using namespace mc::loot;

// Test implementation of IWorld for loot testing
class LootConditionTestWorld : public IWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlock(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override {
        return fluid::Fluid::getFluidState(0);
    }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override { return y >= 0 && y < 256; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] i32 difficulty() const override { return 1; }
};

class LootConditionsTest : public ::testing::Test {
protected:
    LootConditionTestWorld m_world;

    void SetUp() override {
        // 初始化方块和物品系统
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(LootConditionsTest, RandomChanceCondition_Basic) {
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

TEST_F(LootConditionsTest, NotCondition_Basic) {
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

TEST_F(LootConditionsTest, AndCondition_AllTrue) {
    math::Random random(12345);

    auto condition = std::make_unique<AndCondition>();
    condition->addCondition(std::make_unique<RandomChanceCondition>(1.0f));
    condition->addCondition(std::make_unique<RandomChanceCondition>(1.0f));
    condition->addCondition(std::make_unique<RandomChanceCondition>(1.0f));

    EXPECT_TRUE(condition->test(*LootContextBuilder(m_world).withRandom(random).build()));
}

TEST_F(LootConditionsTest, AndCondition_OneFalse) {
    math::Random random(12345);

    auto condition = std::make_unique<AndCondition>();
    condition->addCondition(std::make_unique<RandomChanceCondition>(1.0f));
    condition->addCondition(std::make_unique<RandomChanceCondition>(0.0f));  // 这个为 false
    condition->addCondition(std::make_unique<RandomChanceCondition>(1.0f));

    EXPECT_FALSE(condition->test(*LootContextBuilder(m_world).withRandom(random).build()));
}

TEST_F(LootConditionsTest, OrCondition_AllFalse) {
    math::Random random(12345);

    auto condition = std::make_unique<OrCondition>();
    condition->addCondition(std::make_unique<RandomChanceCondition>(0.0f));
    condition->addCondition(std::make_unique<RandomChanceCondition>(0.0f));
    condition->addCondition(std::make_unique<RandomChanceCondition>(0.0f));

    EXPECT_FALSE(condition->test(*LootContextBuilder(m_world).withRandom(random).build()));
}

TEST_F(LootConditionsTest, OrCondition_OneTrue) {
    math::Random random(12345);

    auto condition = std::make_unique<OrCondition>();
    condition->addCondition(std::make_unique<RandomChanceCondition>(0.0f));
    condition->addCondition(std::make_unique<RandomChanceCondition>(1.0f));  // 这个为 true
    condition->addCondition(std::make_unique<RandomChanceCondition>(0.0f));

    EXPECT_TRUE(condition->test(*LootContextBuilder(m_world).withRandom(random).build()));
}

TEST_F(LootConditionsTest, FortuneCondition_GetLevel) {
    // FortuneCondition::getFortuneLevel 从 LootContext 的 FORTUNE_LEVEL 参数获取
    math::Random random(12345);

    auto context = LootContextBuilder(m_world)
        .withRandom(random)
        .build();

    // 使用 setOwnedValue 设置时运等级
    context->setOwnedValue(LootParams::FORTUNE_LEVEL, 3);

    EXPECT_EQ(FortuneCondition::getFortuneLevel(*context), 3);

    // 测试默认值（未设置时返回 0）
    auto context2 = LootContextBuilder(m_world)
        .withRandom(random)
        .build();

    EXPECT_EQ(FortuneCondition::getFortuneLevel(*context2), 0);
}

TEST_F(LootConditionsTest, FortuneCondition_ApplyBonus) {
    math::Random random(12345);

    // Fortune 0 - 无加成
    EXPECT_EQ(FortuneCondition::applyFortuneBonus(1, 0, random), 1);

    // Fortune I - 大约 33% 概率 +1
    i32 total = 0;
    for (i32 i = 0; i < 1000; ++i) {
        random.setSeed(i);
        total += FortuneCondition::applyFortuneBonus(1, 1, random);
    }
    // 平均应该在 1.33 左右，总和约 1330
    EXPECT_GT(total, 1200);
    EXPECT_LT(total, 1500);

    // Fortune III - 最多 +3
    random.setSeed(12345);
    for (i32 i = 0; i < 1000; ++i) {
        i32 result = FortuneCondition::applyFortuneBonus(1, 3, random);
        EXPECT_GE(result, 1);
        EXPECT_LE(result, 4);  // 1 + 3 = 4
    }
}

// ============================================================================
// LootConditionBuilder 测试
// ============================================================================

TEST(LootConditionBuilderTest, FactoryMethods) {
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
    auto andCondition = LootConditionBuilder::and_(std::move(andConditions));
    EXPECT_NE(andCondition, nullptr);
    EXPECT_EQ(andCondition->getType(), "alternative");

    std::vector<std::unique_ptr<LootCondition>> orConditions;
    orConditions.push_back(LootConditionBuilder::silkTouch());
    orConditions.push_back(LootConditionBuilder::fortune(3));
    auto orCondition = LootConditionBuilder::or_(std::move(orConditions));
    EXPECT_NE(orCondition, nullptr);
    EXPECT_EQ(orCondition->getType(), "or");
}

// ============================================================================
// LootEntry 条件测试
// ============================================================================

class LootEntryConditionTest : public ::testing::Test {
protected:
    LootConditionTestWorld m_world;

    void SetUp() override {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(LootEntryConditionTest, EntryWithCondition) {
    math::Random random(12345);

    // 创建一个带条件的物品条目
    ItemLootEntry entry("minecraft:diamond", RandomValueRange(1.0f, 1.0f), 1, 0);
    entry.addCondition(std::make_unique<RandomChanceCondition>(0.0f));  // 永远不满足

    // 创建 LootContext
    auto context = LootContextBuilder(m_world).withRandom(random).build();

    // 测试条件
    EXPECT_FALSE(entry.testConditions(*context));
}

TEST_F(LootEntryConditionTest, EntryWithMultipleConditions) {
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

TEST_F(LootEntryConditionTest, EntryConditionCloning) {
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

TEST_F(LootEntryConditionTest, PoolWithEntryCondition) {
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

TEST_F(LootConditionsTest, ClonePreservesState) {
    FortuneCondition original(2);
    auto cloned = original.clone();

    EXPECT_EQ(cloned->getType(), "fortune");

    auto* fortuneClone = dynamic_cast<FortuneCondition*>(cloned.get());
    ASSERT_NE(fortuneClone, nullptr);
}

TEST_F(LootConditionsTest, NotConditionWithNullCondition) {
    // NotCondition 应该安全处理 null 内部条件
    NotCondition notNull(nullptr);
    math::Random random(12345);
    auto context = LootContextBuilder(m_world).withRandom(random).build();

    // null 条件被视为 true，取反后为 false
    EXPECT_TRUE(notNull.test(*context));
}

TEST_F(LootConditionsTest, EmptyAndCondition) {
    // 空 AndCondition 应该返回 true（所有 0 个条件都满足）
    AndCondition emptyAnd;
    math::Random random(12345);
    auto context = LootContextBuilder(m_world).withRandom(random).build();

    EXPECT_TRUE(emptyAnd.test(*context));
}

TEST_F(LootConditionsTest, EmptyOrCondition) {
    // 空 OrCondition 应该返回 false（没有条件满足）
    OrCondition emptyOr;
    math::Random random(12345);
    auto context = LootContextBuilder(m_world).withRandom(random).build();

    EXPECT_FALSE(emptyOr.test(*context));
}
