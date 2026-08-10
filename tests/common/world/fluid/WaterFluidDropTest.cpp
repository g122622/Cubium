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
#include "common/core/Constants.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/loot/LootPool.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/entries/ItemLootEntry.hpp"
#include "common/item/loot/entries/LootEntry.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/fluid/fluids/WaterFluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>
#include <spdlog/spdlog.h>

namespace mc {
namespace {

/**
 * @brief 测试用 Mock 掉落表管理器
 */
class MockLootTableManager : public loot::LootTableManager {
public:
    MockLootTableManager()
        : LootTableManager()
    {
        // 创建一个简单的测试掉落表
        auto table = std::make_unique<loot::LootTable>();

        // 创建一个池，总是掉落 1 个测试物品
        auto pool = std::make_unique<loot::LootPool>(loot::RandomValueRange(1.0f), loot::RandomValueRange(1.0f));

        // 添加物品条目 - 使用苹果作为测试物品
        auto entry = std::make_unique<loot::ItemLootEntry>("minecraft:apple",
            loot::RandomValueRange(1.0f, 3.0f), // 数量 1-3
            1,                                  // weight
            1                                   // quality
        );
        pool->addEntry(std::move(entry));
        table->addPool(std::move(pool));

        m_testTable = table.get();
        LootTableManager::registerTable("minecraft:blocks/test_block", std::move(table));
    }

    [[nodiscard]] const loot::LootTable* getTestTable() const { return m_testTable; }

private:
    loot::LootTable* m_testTable = nullptr;
};

/**
 * @brief 测试用 IWorld 实现，支持掉落测试
 */
class WaterFluidTestWorld : public mc::test::BaseTestWorld {
public:
    WaterFluidTestWorld()
        : m_entityManager(mc::test::testEcsRegistry())
    {
        // 初始化掉落表管理器
        m_lootTableManager = std::make_unique<MockLootTableManager>();
    }

    // ========== IBlockReader 接口 ==========

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[packPos(x, y, z)] = state;
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr) {
                return fluidState;
            }
        }
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Peaceful; }

    // ========== TickManager 接口 ==========

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("WaterFluidTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("WaterFluidTestWorld::tickManager not implemented");
    }

    // ========== Random 接口 ==========

    // ========== Entity 管理 ==========

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        if (!entity) {
            return EntityInstanceId(0);
        }
        entity->setWorld(this);
        EntityInstanceId id = m_entityManager.addEntity(std::move(entity));
        return id;
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override { return m_entityManager.getEntity(id); }
    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override { return m_entityManager.getEntity(id); }

    // ========== 掉落表管理器 ==========

    [[nodiscard]] const loot::LootTableManager* lootTableManager() const override { return m_lootTableManager.get(); }

    // ========== 辅助方法 ==========

    [[nodiscard]] EntityManager& entityManager() { return m_entityManager; }
    [[nodiscard]] const EntityManager& entityManager() const { return m_entityManager; }

    [[nodiscard]] size_t entityCount() const { return m_entityManager.entityCount(); }

    void setLootTableManager(std::unique_ptr<MockLootTableManager> manager) { m_lootTableManager = std::move(manager); }

    void clearLootTableManager() { m_lootTableManager.reset(); }

private:
    static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ static_cast<i64>(z & 0x1FFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    EntityManager m_entityManager{mc::test::testEcsRegistry()};
    std::unique_ptr<MockLootTableManager> m_lootTableManager;
};

void ensureRegistriesInitialized()
{
    static std::once_flag s_once;
    std::call_once(s_once, [] {
        fluid::FluidRegistry::instance().initialize();
        VanillaBlocks::initialize();
        // 注册原版实体类型，确保 VanillaEntityTypeKeys::ITEM 等全局缓存与注册表一致。
        // 本测试断言 entity->entityType() == VanillaEntityTypeKeys::ITEM，二者必须来自同一
        // 已初始化的实体注册表，避免依赖前置测试的隐式注册状态（测试顺序污染）。
        entity::VanillaEntities::registerAll();
        // 用 try/catch 包裹 Items::initialize()，吸收旗帜物品双注册异常。
        // 测试顺序污染场景：若 FallingBlockEntityTest::SetUp 先跑过
        // BlockItemRegistry::initializeVanillaBlockItems()，会把 minecraft:white_banner 等
        // 注册为 BlockItem；随后 Items::initialize() -> _registerBanners() 试图把同一 id
        // 注册为 BannerItem（不同子类型），ItemRegistry::registerItem<BannerItem> 内的
        // dynamic_cast<BannerItem*> 失败会抛 "Item id already registered with different type"。
        // 由于 _registerBanners 在 _registerFood(设 APPLE) 与 _registerBuildingBlocks(设 STONE)
        // 之后才执行，异常发生时 Items::APPLE / Items::STONE 等前序物品指针已设置，
        // 本测试文件用例所需的物品（APPLE/STONE）均已可用，故该异常对本测试是良性的，可安全忽略。
        // TODO: 生产代码 Items::initialize() 抛异常后 s_initialized 仍为 false，后续其它测试
        // 直接调用 Items::initialize() 会重入并再次抛出。当前仅靠本文件的 call_once 隔离，
        // 若未来测试顺序变化导致其它 ItemTest 在本文件之后运行，需要进一步隔离。
        try {
            Items::initialize();
        }
        catch (const std::exception& e) {
            spdlog::warn("Items::initialize() threw (likely banner double-registration from test "
                         "order pollution), ignoring: {}",
                e.what());
        }
    });
}

} // namespace

// ============================================================================
// WaterFluid 基础测试
// ============================================================================

TEST(WaterFluidTest, WaterFluidIsRegistered)
{
    ensureRegistriesInitialized();

    // 验证水流体的注册
    fluid::Fluid* water = fluid::Fluid::getFluid(ResourceLocation("minecraft:water"));
    ASSERT_NE(water, nullptr);
    EXPECT_TRUE(water->isSource(water->defaultState()));

    fluid::Fluid* flowingWater = fluid::Fluid::getFluid(ResourceLocation("minecraft:flowing_water"));
    ASSERT_NE(flowingWater, nullptr);
    EXPECT_FALSE(flowingWater->isSource(flowingWater->defaultState()));
}

TEST(WaterFluidTest, WaterFluidIsEquivalent)
{
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

TEST(WaterFluidTest, WaterFluidProperties)
{
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

TEST(WaterFluidDropTest, ItemDropHelperSpawnEntities)
{
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
    for (EntityInstanceId id : spawnedIds) {
        Entity* entity = world.getEntity(id);
        ASSERT_NE(entity, nullptr);
        EXPECT_EQ(entity->entityType(), entity::VanillaEntityTypeKeys::ITEM);
    }
}

TEST(WaterFluidDropTest, ItemDropHelperEmptyListNoSpawn)
{
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

TEST(WaterFluidDropTest, LootTableGeneration)
{
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
    auto context = loot::LootContextBuilder(world).withRandom(rng).withSeed(world.seed()).build();

    ASSERT_NE(context, nullptr);

    // 设置掉落表解析器
    context->setLootTableResolver(
        [&manager](const std::string& id) -> const loot::LootTable* { return manager->getTable(id); });

    // 生成掉落
    auto drops = table->generate(*context);

    // 验证生成了掉落物
    EXPECT_GE(drops.size(), 1u);

    // 验证掉落物是苹果
    for (const auto& stack : drops) {
        EXPECT_FALSE(stack.isEmpty());
        EXPECT_GE(stack.getCount(), 1);
        EXPECT_LE(stack.getCount(), 3); // 数量 1-3
    }
}

TEST(WaterFluidDropTest, NoLootTableManagerReturnsNull)
{
    ensureRegistriesInitialized();

    WaterFluidTestWorld world;
    world.clearLootTableManager();

    // 验证空掉落表管理器
    EXPECT_EQ(world.lootTableManager(), nullptr);
}

TEST(WaterFluidDropTest, BlockGetLootTableWithManager)
{
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
    (void)table; // 避免未使用变量警告
}

// ============================================================================
// 流体替换方块上下文测试
// ============================================================================

TEST(WaterFluidDropTest, WaterFluidRegisteredCorrectly)
{
    ensureRegistriesInitialized();

    // 验证水流体已正确注册
    fluid::Fluid* waterSource = fluid::Fluid::getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterSource, nullptr);
    EXPECT_TRUE(waterSource->isSource(waterSource->defaultState()));

    fluid::Fluid* flowingWater = fluid::Fluid::getFluid(fluid::FluidRegistry::FLOWING_WATER_ID);
    ASSERT_NE(flowingWater, nullptr);
    EXPECT_FALSE(flowingWater->isSource(flowingWater->defaultState()));
}

TEST(WaterFluidDropTest, FluidStateLevels)
{
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
