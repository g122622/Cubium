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
#include "world/fluid/Fluid.hpp"
#include "util/math/random/Random.hpp"
#include "core/Constants.hpp"

using namespace mc;
using namespace mc::loot;

// Test implementation of IWorld for loot testing
class LootTestWorld : public IWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlock(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override {
        return fluid::Fluid::getFluidState(0);
    }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override { return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT; }
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
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
};

class LootTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
    }

    LootTestWorld m_world;
};

// RandomValueRange Tests
TEST_F(LootTest, RandomValueRange_FixedValue) {
    RandomValueRange range(5.0f);
    math::Random rng(12345);
    EXPECT_EQ(5, range.generateInt(rng));
    EXPECT_FLOAT_EQ(5.0f, range.generateFloat(rng));
    EXPECT_TRUE(range.isFixed());
}

TEST_F(LootTest, RandomValueRange_Range) {
    RandomValueRange range(1.0f, 10.0f);
    math::Random rng(12345);
    for (int i = 0; i < 10; ++i) {
        i32 value = range.generateInt(rng);
        EXPECT_GE(value, 1);
        EXPECT_LE(value, 10);
    }
}

// BinomialRange Tests
TEST_F(LootTest, BinomialRange_Basic) {
    BinomialRange range(10, 0.5f);
    math::Random rng(12345);
    for (int i = 0; i < 10; ++i) {
        i32 value = range.generateInt(rng);
        EXPECT_GE(value, 0);
        EXPECT_LE(value, 10);
    }
}

TEST_F(LootTest, BinomialRange_ZeroProbability) {
    BinomialRange range(10, 0.0f);
    math::Random rng(12345);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(0, range.generateInt(rng));
    }
}

TEST_F(LootTest, BinomialRange_FullProbability) {
    BinomialRange range(10, 1.0f);
    math::Random rng(12345);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(10, range.generateInt(rng));
    }
}

// LootContext Tests
TEST_F(LootTest, LootContext_Builder) {
    math::Random rng(12345);
    IWorld& worldRef = m_world;
    auto context = LootContextBuilder(worldRef)
        .withRandom(rng)
        .withLuck(1.5f)
        .build();

    ASSERT_NE(context, nullptr);
    EXPECT_FLOAT_EQ(1.5f, context->getLuck());
}

TEST_F(LootTest, LootContext_LootingModifier) {
    math::Random rng(12345);
    IWorld& worldRef = m_world;
    auto context = LootContextBuilder(worldRef)
        .withRandom(rng)
        .withLootingModifier(3)
        .build();

    ASSERT_NE(context, nullptr);
    EXPECT_EQ(3, context->getLootingModifier());
}

// LootEntry Tests
TEST_F(LootTest, EmptyLootEntry_GenerateNothing) {
    EmptyLootEntry entry;
    math::Random rng(12345);
    IWorld& worldRef = m_world;
    auto context = LootContextBuilder(worldRef).withRandom(rng).build();

    std::vector<ItemStack> items;
    bool success = entry.generate([&items](const ItemStack& stack) {
        items.push_back(stack);
    }, *context);

    EXPECT_TRUE(success);
    EXPECT_TRUE(items.empty());
}

TEST_F(LootTest, ItemLootEntry_Weight) {
    ItemLootEntry entry("minecraft:porkchop", RandomValueRange(1.0f), 10, 2);
    EXPECT_EQ(10, entry.getWeight());
    EXPECT_EQ(2, entry.getQuality());
    EXPECT_EQ(10, entry.getEffectiveWeight(0.0f));
    EXPECT_EQ(12, entry.getEffectiveWeight(1.0f));
    EXPECT_EQ(8, entry.getEffectiveWeight(-1.0f));
}

// LootTable Tests
TEST_F(LootTest, LootTable_Empty) {
    LootTable table;
    math::Random rng(12345);
    IWorld& worldRef = m_world;
    auto context = LootContextBuilder(worldRef).withRandom(rng).build();

    auto items = table.generate(*context);
    EXPECT_TRUE(items.empty());
}

// LootTableManager Tests
TEST_F(LootTest, LootTableManager_RegisterAndGet) {
    LootTableManager manager;
    auto table = std::make_unique<LootTable>();
    table->addPool(std::make_unique<LootPool>(RandomValueRange(1.0f)));
    manager.registerTable("test:pig", std::move(table));

    EXPECT_TRUE(manager.hasTable("test:pig"));
    EXPECT_FALSE(manager.hasTable("test:cow"));

    const LootTable* retrieved = manager.getTable("test:pig");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(static_cast<size_t>(1), retrieved->poolCount());
}

TEST_F(LootTest, LootTableManager_DefaultTables) {
    LootTableManager manager;
    manager.initializeDefaultTables();

    EXPECT_TRUE(manager.hasTable("minecraft:entities/pig"));
    EXPECT_TRUE(manager.hasTable("minecraft:entities/cow"));
    EXPECT_TRUE(manager.hasTable("minecraft:entities/sheep"));
    EXPECT_TRUE(manager.hasTable("minecraft:entities/chicken"));
}
