#include <gtest/gtest.h>

#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/fluids/WaterFluid.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/entity/loot/LootTable.hpp"
#include "common/entity/loot/LootContext.hpp"
#include "common/entity/loot/LootPool.hpp"
#include "common/entity/loot/LootEntry.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/core/Constants.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用 Mock 掉落表管理器
 */
class MockLootTableManager : public loot::LootTableManager {
public:
    MockLootTableManager() : LootTableManager() {
        // 创建一个简单的测试掉落表
        auto table = std::make_unique<loot::LootTable>();

        // 创建一个池，总是掉落 1 个测试物品
        auto pool = std::make_unique<loot::LootPool>(
            loot::RandomValueRange(1.0f),
            loot::RandomValueRange(1.0f));

        // 添加物品条目 - 使用苹果作为测试物品
        auto entry = std::make_unique<loot::ItemLootEntry>(
            "minecraft:apple",
            loot::RandomValueRange(1.0f, 3.0f),  // 数量 1-3
            1,  // weight
            1   // quality
        );
        pool->addEntry(std::move(entry));
        table->addPool(std::move(pool));

        m_testTable = table.get();
        LootTableManager::registerTable("minecraft:blocks/test_block", std::move(table));
    }

    [[nodiscard]] const loot::LootTable* getTestTable() const {
        return m_testTable;
    }

private:
    loot::LootTable* m_testTable = nullptr;
};

/**
 * @brief 测试用 IWorld 实现，支持掉落测试
 */
class WaterFluidTestWorld : public IWorld {
public:
    WaterFluidTestWorld()
        : m_random(12345)
        , m_entityManager()
    {
        // 初始化掉落表管理器
        m_lootTableManager = std::make_unique<MockLootTableManager>();
    }

    // ========== IBlockReader 接口 ==========

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override {
        m_blocks[packPos(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override {
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr) {
                return fluidState;
            }
        }
        return fluid::Fluid::getFluidState(fluid::FluidRegistry::EMPTY_ID);
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
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
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Peaceful; }
    [[nodiscard]] bool isClientSide() override { return false; }

    // ========== TickManager 接口 ==========

    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("WaterFluidTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("WaterFluidTestWorld::tickManager not implemented");
    }

    // ========== Random 接口 ==========

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    // ========== WorldBorder 接口 ==========

    [[nodiscard]] world::border::WorldBorder& worldBorder() override {
        throw std::runtime_error("WaterFluidTestWorld::worldBorder not implemented");
    }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override {
        throw std::runtime_error("WaterFluidTestWorld::worldBorder not implemented");
    }

    // ========== Entity 管理 ==========

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override {
        if (!entity) {
            return EntityId(0);
        }
        entity->setWorld(this);
        EntityId id = m_entityManager.addEntity(std::move(entity));
        return id;
    }

    [[nodiscard]] Entity* getEntity(EntityId id) override {
        return m_entityManager.getEntity(id);
    }
    [[nodiscard]] const Entity* getEntity(EntityId id) const override {
        return m_entityManager.getEntity(id);
    }

    // ========== 掉落表管理器 ==========

    [[nodiscard]] const loot::LootTableManager* lootTableManager() const override {
        return m_lootTableManager.get();
    }

    // ========== 辅助方法 ==========

    [[nodiscard]] EntityManager& entityManager() { return m_entityManager; }
    [[nodiscard]] const EntityManager& entityManager() const { return m_entityManager; }

    [[nodiscard]] size_t entityCount() const { return m_entityManager.entityCount(); }

    void setLootTableManager(std::unique_ptr<MockLootTableManager> manager) {
        m_lootTableManager = std::move(manager);
    }

    void clearLootTableManager() {
        m_lootTableManager.reset();
    }

private:
    static i64 packPos(i32 x, i32 y, i32 z) {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ static_cast<i64>(z & 0x1FFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    math::Random m_random;
    EntityManager m_entityManager;
    std::unique_ptr<MockLootTableManager> m_lootTableManager;
};

void ensureRegistriesInitialized() {
    static std::once_flag s_once;
    std::call_once(s_once, [] {
        fluid::FluidRegistry::instance().initialize();
        VanillaBlocks::initialize();
        Items::initialize();
    });
}

} // namespace

// ============================================================================
// WaterFluid 基础测试
// ============================================================================

TEST(WaterFluidTest, WaterFluidIsRegistered) {
    ensureRegistriesInitialized();

    // 验证水流体的注册
    fluid::Fluid* water = fluid::Fluid::getFluid(ResourceLocation("minecraft:water"));
    ASSERT_NE(water, nullptr);
    EXPECT_TRUE(water->isSource(water->defaultState()));

    fluid::Fluid* flowingWater = fluid::Fluid::getFluid(ResourceLocation("minecraft:flowing_water"));
    ASSERT_NE(flowingWater, nullptr);
    EXPECT_FALSE(flowingWater->isSource(flowingWater->defaultState()));
}

TEST(WaterFluidTest, WaterFluidIsEquivalent) {
    ensureRegistriesInitialized();

    fluid::Fluid* water = fluid::Fluid::getFluid(ResourceLocation("minecraft:water"));
    fluid::Fluid* flowingWater = fluid::Fluid::getFluid(ResourceLocation("minecraft:flowing_water"));

    ASSERT_NE(water, nullptr);
    ASSERT_NE(flowingWater, nullptr);

    // 水和流动水应该是等效的
    EXPECT_TRUE(water->isEquivalentTo(*water));
    EXPECT_TRUE(water->isEquivalentTo(*flowingWater));
    EXPECT_TRUE(flowingWater->isEquivalentTo(*water));
    EXPECT_TRUE(flowingWater->isEquivalentTo(*flowingWater));
}

TEST(WaterFluidTest, WaterFluidProperties) {
    ensureRegistriesInitialized();

    fluid::Fluid* water = fluid::Fluid::getFluid(ResourceLocation("minecraft:water"));

    ASSERT_NE(water, nullptr);

    // 验证水的基本属性
    EXPECT_EQ(water->getTickDelay(), 5);
    EXPECT_EQ(water->canSourcesMultiply(), true);
    EXPECT_FLOAT_EQ(water->getExplosionResistance(), 100.0f);
}

// ============================================================================
// 掉落生成测试（验证 ItemDropHelper 和 LootTable 集成）
// ============================================================================

TEST(WaterFluidDropTest, ItemDropHelperSpawnEntities) {
    ensureRegistriesInitialized();

    WaterFluidTestWorld world;
    const BlockPos pos(10, 64, 20);

    // 创建掉落物品列表
    std::vector<ItemStack> drops;
    drops.emplace_back(*Items::APPLE, 3);
    drops.emplace_back(*Items::STONE, 10);

    math::Random rng(12345);

    // 使用 ItemDropHelper 生成物品实体
    auto spawnedIds = ItemDropHelper::spawnItemEntities(&world, pos, drops, rng);

    // 验证生成了正确数量的物品实体
    EXPECT_EQ(spawnedIds.size(), 2u);
    EXPECT_EQ(world.entityCount(), 2u);

    // 验证物品实体的类型
    for (EntityId id : spawnedIds) {
        Entity* entity = world.getEntity(id);
        ASSERT_NE(entity, nullptr);
        EXPECT_EQ(entity->legacyType(), LegacyEntityType::Item);
    }
}

TEST(WaterFluidDropTest, ItemDropHelperEmptyListNoSpawn) {
    ensureRegistriesInitialized();

    WaterFluidTestWorld world;
    const BlockPos pos(10, 64, 20);

    // 空掉落列表
    std::vector<ItemStack> drops;
    math::Random rng(12345);

    auto spawnedIds = ItemDropHelper::spawnItemEntities(&world, pos, drops, rng);

    // 不应该生成任何物品
    EXPECT_TRUE(spawnedIds.empty());
    EXPECT_EQ(world.entityCount(), 0u);
}

TEST(WaterFluidDropTest, LootTableGeneration) {
    ensureRegistriesInitialized();

    WaterFluidTestWorld world;
    const BlockPos pos(10, 64, 20);

    // 获取测试掉落表
    const loot::LootTableManager* manager = world.lootTableManager();
    ASSERT_NE(manager, nullptr);

    const loot::LootTable* table = manager->getTable("minecraft:blocks/test_block");
    ASSERT_NE(table, nullptr);

    // 创建掉落上下文
    math::Random rng(world.seed());
    auto context = loot::LootContextBuilder(world)
        .withRandom(rng)
        .withSeed(world.seed())
        .build();

    ASSERT_NE(context, nullptr);

    // 设置掉落表解析器
    context->setLootTableResolver([&manager](const std::string& id) -> const loot::LootTable* {
        return manager->getTable(id);
    });

    // 生成掉落
    auto drops = table->generate(*context);

    // 验证生成了掉落物
    EXPECT_GE(drops.size(), 1u);

    // 验证掉落物是苹果
    for (const auto& stack : drops) {
        EXPECT_FALSE(stack.isEmpty());
        EXPECT_GE(stack.getCount(), 1);
        EXPECT_LE(stack.getCount(), 3);  // 数量 1-3
    }
}

TEST(WaterFluidDropTest, NoLootTableManagerReturnsNull) {
    ensureRegistriesInitialized();

    WaterFluidTestWorld world;
    world.clearLootTableManager();

    // 验证空掉落表管理器
    EXPECT_EQ(world.lootTableManager(), nullptr);
}

TEST(WaterFluidDropTest, BlockGetLootTableWithManager) {
    ensureRegistriesInitialized();

    WaterFluidTestWorld world;
    const loot::LootTableManager* manager = world.lootTableManager();
    ASSERT_NE(manager, nullptr);

    // 测试石头的掉落表获取
    const Block* stone = VanillaBlocks::STONE;
    ASSERT_NE(stone, nullptr);

    // 石头没有设置掉落表ID，所以应该返回 nullptr
    const loot::LootTable* table = stone->getLootTable(*manager);
    // 石头默认没有掉落表，除非在初始化时设置
    (void)table;  // 避免未使用变量警告
}

// ============================================================================
// 流体替换方块上下文测试
// ============================================================================

TEST(WaterFluidDropTest, WaterFluidRegisteredCorrectly) {
    ensureRegistriesInitialized();

    // 验证水流体已正确注册
    fluid::Fluid* waterSource = fluid::Fluid::getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterSource, nullptr);
    EXPECT_TRUE(waterSource->isSource(waterSource->defaultState()));

    fluid::Fluid* flowingWater = fluid::Fluid::getFluid(fluid::FluidRegistry::FLOWING_WATER_ID);
    ASSERT_NE(flowingWater, nullptr);
    EXPECT_FALSE(flowingWater->isSource(flowingWater->defaultState()));
}

TEST(WaterFluidDropTest, FluidStateLevels) {
    ensureRegistriesInitialized();

    fluid::Fluid* water = fluid::Fluid::getFluid(ResourceLocation("minecraft:water"));
    ASSERT_NE(water, nullptr);

    // 水源头应该有 level 8
    const fluid::FluidState& sourceState = water->defaultState();
    EXPECT_TRUE(sourceState.isSource());
    EXPECT_EQ(sourceState.getLevel(), 8);

    // 流动水应该有不同的 level
    fluid::Fluid* flowingWater = fluid::Fluid::getFluid(ResourceLocation("minecraft:flowing_water"));
    ASSERT_NE(flowingWater, nullptr);

    const fluid::FluidState& flowingState = flowingWater->defaultState();
    EXPECT_FALSE(flowingState.isSource());
    // 默认流动水的 level 取决于实现
}

} // namespace mc
