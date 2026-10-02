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

// 水下植物不被水流替换的回归测试。
//
// 缺陷背景：KelpBlock / KelpPlantBlock / SeagrassBlock / TallSeagrassBlock 在 Cubium 里
// 注册为 `.noCollision().notSolid()`，而 FlowingFluid::isBlocked 对**未实现 ILiquidContainer**
// 的方块只按 `canBeReplacedByFluid()`（= canBeReplaced || !isSolid）判定，于是它们被判为
// "可被流体替换"。水流每次 tick 都把这些植物当作被冲毁的方块，经 WaterFluid::beforeReplacingBlock
// 当作"方块被水破坏"生成一次掉落物——实测单次会话堆积 19326 个物品实体。
//
// 原版语义：这四类方块都实现 LiquidBlockContainer 且 canPlaceLiquid/placeLiquid 恒返 false，
// 故流体既不能灌入、也不能替换它们。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/loot/LootPool.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/entries/ItemLootEntry.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/ILiquidContainer.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/fluid/fluids/WaterFluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <map>
#include <memory>
#include <utility>

using namespace mc;
using namespace mc::block_registry;

namespace {

/// 掉落表管理器桩：为海带/海草注册"必掉 1 个自身"的掉落表，使"水流冲毁植物"若真的发生
/// 必然留下可观测的物品实体。
class SealedPlantLootTableManager : public loot::LootTableManager {
public:
    void registerSingleDropTable(const std::string& tableId, const std::string& itemId)
    {
        auto table = std::make_unique<loot::LootTable>();
        auto pool = std::make_unique<loot::LootPool>(loot::RandomValueRange(1.0f), loot::RandomValueRange(1.0f));
        pool->addEntry(std::make_unique<loot::ItemLootEntry>(itemId, loot::RandomValueRange(1.0f, 1.0f), 1, 1));
        table->addPool(std::move(pool));
        registerTable(tableId, std::move(table));
    }
};

/**
 * @brief 水下植物流体密封性测试用世界
 *
 * 提供方块状态存储、TickManager、掉落表管理器与实体生成，使完整的水流 tick → 替换方块
 * → 生成掉落物链路可在单测内驱动。
 */
class SealedPlantTestWorld final : public mc::test::BaseTestWorld {
public:
    SealedPlantTestWorld()
        : m_entityManager(mc::test::testEcsRegistry())
    {
        m_lootTableManager = std::make_unique<SealedPlantLootTableManager>();
        m_lootTableManager->registerSingleDropTable("minecraft:blocks/kelp", "minecraft:kelp");
        m_lootTableManager->registerSingleDropTable("minecraft:blocks/kelp_plant", "minecraft:kelp");
        m_lootTableManager->registerSingleDropTable("minecraft:blocks/seagrass", "minecraft:seagrass");
        m_lootTableManager->registerSingleDropTable("minecraft:blocks/tall_seagrass", "minecraft:seagrass");
        m_lootTableManager->registerSingleDropTable("minecraft:blocks/test_block", "minecraft:kelp");
    }

    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        return it != m_blocks.end() ? it->second : nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
            return true;
        }
        // 规范化为注册表持有的状态指针，避免存入调用方栈帧上的临时状态
        const BlockState* canonical = BlockRegistry::instance().getBlockState(state->stateId());
        m_blocks[pos] = canonical != nullptr ? canonical : state;
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        return setBlockState(x, y, z, state);
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

    [[nodiscard]] mc::world::chunk::IChunkManager* chunkManager() override { return &m_stubChunks; }
    [[nodiscard]] const mc::world::chunk::IChunkManager* chunkManager() const override { return &m_stubChunks; }
    mc::test::StubChunkManager m_stubChunks{nullptr, true};

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
        return *m_tickManagerPtr;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        return const_cast<SealedPlantTestWorld*>(this)->tickManager();
    }

    [[nodiscard]] const loot::LootTableManager* lootTableManager() const override { return m_lootTableManager.get(); }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        if (!entity) {
            return EntityInstanceId(0);
        }
        entity->setWorld(this);
        return m_entityManager.addEntity(std::move(entity));
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override { return m_entityManager.getEntity(id); }
    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override { return m_entityManager.getEntity(id); }

    [[nodiscard]] size_t entityCount() const { return m_entityManager.entityCount(); }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    EntityManager m_entityManager;
    std::unique_ptr<SealedPlantLootTableManager> m_lootTableManager;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
};

void ensureRegistriesInitialized()
{
    static std::once_flag s_once;
    std::call_once(s_once, [] {
        fluid::FluidRegistry::instance().initialize();
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
    });
}

/// 取水源流体实例（FluidRegistry::WATER_ID）
[[nodiscard]] fluid::FlowingFluid* waterFlowing()
{
    auto* source = dynamic_cast<fluid::WaterSourceFluid*>(fluid::Fluid::getFluid(fluid::FluidRegistry::WATER_ID));
    return source != nullptr ? &source->getFlowing() : nullptr;
}

} // namespace

// ============================================================================
// 接口归属：四类水下植物都实现 ILiquidContainer
// ============================================================================

TEST(SealedPlantLiquidTest, UnderwaterPlantsImplementLiquidContainer)
{
    ensureRegistriesInitialized();

    const Block* plants[] = {
        VanillaBlocks::KELP, VanillaBlocks::KELP_PLANT, VanillaBlocks::SEAGRASS, VanillaBlocks::TALL_SEAGRASS};
    for (const Block* plant : plants) {
        ASSERT_NE(plant, nullptr) << plant->blockLocation().toString();
        EXPECT_NE(dynamic_cast<const ILiquidContainer*>(plant), nullptr)
            << plant->blockLocation().toString() << " 未实现 ILiquidContainer，会被水流当作可替换方块";
    }
}

// ============================================================================
// 容器语义：canContainFluid / receiveFluid / containsFluid 全部拒绝
// ============================================================================

TEST(SealedPlantLiquidTest, SealedPlantsRejectWater)
{
    ensureRegistriesInitialized();

    SealedPlantTestWorld world;
    fluid::Fluid* water = fluid::Fluid::getFluid(fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(water, nullptr);

    const BlockPos pos(3, 64, 5);
    const Block* plants[] = {
        VanillaBlocks::KELP, VanillaBlocks::KELP_PLANT, VanillaBlocks::SEAGRASS, VanillaBlocks::TALL_SEAGRASS};
    for (const Block* plant : plants) {
        auto* container = dynamic_cast<ILiquidContainer*>(const_cast<Block*>(plant));
        ASSERT_NE(container, nullptr);

        const BlockState& state = plant->defaultState();
        EXPECT_FALSE(container->canContainFluid(world, pos, state, *water)) << plant->blockLocation().toString();
        EXPECT_FALSE(container->receiveFluid(world, pos, state, water->defaultState()))
            << plant->blockLocation().toString();
        EXPECT_FALSE(container->containsFluid(world, pos, state)) << plant->blockLocation().toString();
    }
}

// ============================================================================
// 行为回归：水流 tick 不得冲毁海带 / 海草，也不得生成掉落物
// ============================================================================

TEST(SealedPlantLiquidTest, WaterFlowDoesNotDestroyKelpOrSpawnDrops)
{
    ensureRegistriesInitialized();

    fluid::FlowingFluid* water = waterFlowing();
    ASSERT_NE(water, nullptr);
    ASSERT_NE(VanillaBlocks::KELP, nullptr);
    ASSERT_NE(VanillaBlocks::KELP_PLANT, nullptr);

    const BlockPos kelpPos(4, 64, 4);
    const BlockPos waterPos(3, 64, 4); // 与海带水平相邻的水格

    // 水位下方铺石头，使水流无法向下流（canFlowDown 为假），从而必须侧向扩散到海带格；
    // 否则水流会先往下方流走，getHorizontalSourceCount 不足 3 且不再侧扩，测不到目标分支。
    SealedPlantTestWorld world;
    world.setBlockAt(kelpPos, &VanillaBlocks::KELP->defaultState());
    world.setBlockAt(BlockPos(kelpPos.x, kelpPos.y - 1, kelpPos.z), &VanillaBlocks::STONE->defaultState());
    world.setBlockAt(BlockPos(waterPos.x, waterPos.y - 1, waterPos.z), &VanillaBlocks::STONE->defaultState());
    world.setBlockAt(waterPos, &VanillaBlocks::WATER->defaultState());

    const BlockState* before = world.getBlockState(kelpPos.x, kelpPos.y, kelpPos.z);
    ASSERT_NE(before, nullptr);
    ASSERT_TRUE(before->is(VanillaBlocks::KELP));

    // 驱动一格水流的完整 tick（flowAround + spreadHorizontally）
    const fluid::FluidState* waterState = world.getFluidState(waterPos.x, waterPos.y, waterPos.z);
    ASSERT_NE(waterState, nullptr);
    fluid::FluidState mutableWater = *waterState;
    water->tick(world, waterPos, mutableWater);

    const BlockState* after = world.getBlockState(kelpPos.x, kelpPos.y, kelpPos.z);
    ASSERT_NE(after, nullptr) << "海带被水流冲毁";
    EXPECT_TRUE(after->is(VanillaBlocks::KELP)) << "海带被水流替换为 " << after->toString();
    EXPECT_EQ(world.entityCount(), 0u) << "水流冲毁海带并生成了掉落物";
}

TEST(SealedPlantLiquidTest, WaterFlowDoesNotDestroySeagrassOrSpawnDrops)
{
    ensureRegistriesInitialized();

    fluid::FlowingFluid* water = waterFlowing();
    ASSERT_NE(water, nullptr);
    ASSERT_NE(VanillaBlocks::SEAGRASS, nullptr);

    const BlockPos grassPos(4, 64, 4);
    const BlockPos waterPos(3, 64, 4);

    SealedPlantTestWorld world;
    world.setBlockAt(grassPos, &VanillaBlocks::SEAGRASS->defaultState());
    world.setBlockAt(BlockPos(grassPos.x, grassPos.y - 1, grassPos.z), &VanillaBlocks::STONE->defaultState());
    world.setBlockAt(BlockPos(waterPos.x, waterPos.y - 1, waterPos.z), &VanillaBlocks::STONE->defaultState());
    world.setBlockAt(waterPos, &VanillaBlocks::WATER->defaultState());

    const fluid::FluidState* waterState = world.getFluidState(waterPos.x, waterPos.y, waterPos.z);
    ASSERT_NE(waterState, nullptr);
    fluid::FluidState mutableWater = *waterState;
    water->tick(world, waterPos, mutableWater);

    const BlockState* after = world.getBlockState(grassPos.x, grassPos.y, grassPos.z);
    ASSERT_NE(after, nullptr) << "海草被水流冲毁";
    EXPECT_TRUE(after->is(VanillaBlocks::SEAGRASS)) << "海草被水流替换为 " << after->toString();
    EXPECT_EQ(world.entityCount(), 0u) << "水流冲毁海草并生成了掉落物";
}

// ============================================================================
// 对照组：真正可被替换的方块仍应被水流冲毁并留下掉落物
// ============================================================================

TEST(SealedPlantLiquidTest, WaterFlowStillDestroysReplaceablePlants)
{
    ensureRegistriesInitialized();

    fluid::FlowingFluid* water = waterFlowing();
    ASSERT_NE(water, nullptr);
    ASSERT_NE(NaturalBlocks::DEAD_BUSH, nullptr);

    // 枯灌木是 replaceable + notSolid 且未实现 ILiquidContainer，属"确应被水流冲毁"的方块。
    // 本用例确保上面的修复没有把整条"水流冲毁方块 + 掉落"链路一并关掉。
    const BlockPos bushPos(4, 64, 4);
    const BlockPos waterPos(3, 64, 4);

    SealedPlantTestWorld world;
    world.setBlockAt(bushPos, &NaturalBlocks::DEAD_BUSH->defaultState());
    world.setBlockAt(BlockPos(bushPos.x, bushPos.y - 1, bushPos.z), &VanillaBlocks::STONE->defaultState());
    world.setBlockAt(BlockPos(waterPos.x, waterPos.y - 1, waterPos.z), &VanillaBlocks::STONE->defaultState());
    world.setBlockAt(waterPos, &VanillaBlocks::WATER->defaultState());

    const fluid::FluidState* waterState = world.getFluidState(waterPos.x, waterPos.y, waterPos.z);
    ASSERT_NE(waterState, nullptr);
    fluid::FluidState mutableWater = *waterState;
    water->tick(world, waterPos, mutableWater);

    const BlockState* after = world.getBlockState(bushPos.x, bushPos.y, bushPos.z);
    // 枯灌木没有掉落表（掉落表管理器只注册了海带/海草），故此处只断言"被替换为水流方块"。
    // 若它仍完好，说明水流根本没有扩散到侧面，本用例的对照组失去意义。
    ASSERT_NE(after, nullptr);
    EXPECT_FALSE(after->is(NaturalBlocks::DEAD_BUSH))
        << "水流未冲毁可替换方块，对照条件不成立（测试环境未触发侧向扩散）";
}
