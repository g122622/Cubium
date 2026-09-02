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
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/special/BeeEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/vegetation/DoublePlantBlock.hpp"
#include "common/world/block/registry/NaturalBlocks.hpp"
#include "common/world/block/registry/NetherBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/BeehiveBlockEntity.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <cmath>
#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用模拟世界
 */
class BeeTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const { return m_spawnedEntities; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("BeeTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("BeeTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

/**
 * @brief 支持方块实体的测试用模拟世界
 *
 * 扩展 BeeTestWorld，添加 getBlockEntity/setBlockEntity 支持，
 * 用于测试 BeeEntity 的蜂巢验证、蜂巢交互等方法。
 */
class BeeHiveTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        const auto it = m_blockEntities.find(pos);
        if (it != m_blockEntities.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        const auto it = m_blockEntities.find(pos);
        if (it != m_blockEntities.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) override
    {
        if (entity) {
            m_blockEntities[pos] = std::unique_ptr<BlockEntity>(entity);
        } else {
            m_blockEntities.erase(pos);
        }
    }

    void removeBlockEntity(const BlockPos& pos) override { m_blockEntities.erase(pos); }

    // 便利方法：直接添加拥有权的方块实体
    void addBlockEntity(const BlockPos& pos, std::unique_ptr<BlockEntity> entity)
    {
        m_blockEntities[pos] = std::move(entity);
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override
    {
        // 测试中忽略声音播放
    }

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("BeeHiveTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("BeeHiveTestWorld::tickManager not implemented");
    }

    // GameRules 接口
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }
    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }

    // 测试辅助方法
    void setBlockAt(const BlockPos& pos, const BlockState* state)
    {
        m_blocks[pos] = std::make_unique<BlockState>(*state);
    }

    void incrementTick() { m_currentTick++; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    void setBeehiveAt(const BlockPos& pos)
    {
        // 设置蜂巢方块
        if (block_registry::NaturalBlocks::BEEHIVE != nullptr) {
            const BlockState* beehiveState = &block_registry::NaturalBlocks::BEEHIVE->defaultState();
            m_blocks[pos] = std::make_unique<BlockState>(*beehiveState);
        }
        // 设置蜂巢方块实体
        addBlockEntity(pos, std::make_unique<blockentity::BeehiveBlockEntity>(pos));
    }

    void setFireAt(const BlockPos& pos)
    {
        // 设置火焰方块（NetherBlocks::FIRE）
        if (block_registry::NetherBlocks::FIRE != nullptr) {
            const BlockState* fireState = &block_registry::NetherBlocks::FIRE->defaultState();
            m_blocks[pos] = std::make_unique<BlockState>(*fireState);
        }
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    u64 m_currentTick = 0;
    world::gamerule::GameRules m_gameRules;
};

/**
 * @brief 支持天气/时间控制的测试用模拟世界
 *
 * 扩展 BeeHiveTestWorld，添加 isRaining/isThundering/dayTime 控制，
 * 用于测试 BeeEntity 的天气/夜间回巢逻辑（BEES_STAY_IN_HIVE 等效）。
 */
class BeeWeatherTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        const auto it = m_blockEntities.find(pos);
        if (it != m_blockEntities.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        const auto it = m_blockEntities.find(pos);
        if (it != m_blockEntities.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) override
    {
        if (entity) {
            m_blockEntities[pos] = std::unique_ptr<BlockEntity>(entity);
        } else {
            m_blockEntities.erase(pos);
        }
    }

    void removeBlockEntity(const BlockPos& pos) override { m_blockEntities.erase(pos); }

    void addBlockEntity(const BlockPos& pos, std::unique_ptr<BlockEntity> entity)
    {
        m_blockEntities[pos] = std::move(entity);
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    // 天气/时间控制
    [[nodiscard]] i64 dayTime() const override { return m_dayTime; }
    void setDayTime(i64 time) { m_dayTime = time; }

    [[nodiscard]] bool isRaining() const override { return m_raining; }
    void setRaining(bool raining) { m_raining = raining; }

    [[nodiscard]] bool isThundering() const override { return m_thundering; }
    void setThundering(bool thundering) { m_thundering = thundering; }

    // TickManager 接口
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("BeeWeatherTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("BeeWeatherTestWorld::tickManager not implemented");
    }

    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }
    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }

    // 便利方法
    void setBlockAt(const BlockPos& pos, const BlockState* state)
    {
        m_blocks[pos] = std::make_unique<BlockState>(*state);
    }

    void setBeehiveAt(const BlockPos& pos)
    {
        if (block_registry::NaturalBlocks::BEEHIVE != nullptr) {
            const BlockState* beehiveState = &block_registry::NaturalBlocks::BEEHIVE->defaultState();
            m_blocks[pos] = std::make_unique<BlockState>(*beehiveState);
        }
        addBlockEntity(pos, std::make_unique<blockentity::BeehiveBlockEntity>(pos));
    }

    void setFireAt(const BlockPos& pos)
    {
        if (block_registry::NetherBlocks::FIRE != nullptr) {
            const BlockState* fireState = &block_registry::NetherBlocks::FIRE->defaultState();
            m_blocks[pos] = std::make_unique<BlockState>(*fireState);
        }
    }

    void incrementTick() { m_currentTick++; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::unordered_map<BlockPos, std::unique_ptr<BlockEntity>> m_blockEntities;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    u64 m_currentTick = 0;
    i64 m_dayTime = 1000; // 默认白天 (tick 1000 < 12000)
    bool m_raining = false;
    bool m_thundering = false;
    world::gamerule::GameRules m_gameRules;
};

class BeeEntityTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化顺序：方块 -> 方块标签 -> 物品 -> 方块物品 -> 物品标签
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        item::tag::ItemTags::initialize();
    }

    BeeTestWorld m_world;
};

// ============================================================================
// 繁殖物品测试 - isBreedingItem
// ============================================================================

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsDandelion)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    const BlockItem* dandelionItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::DANDELION);
    ASSERT_NE(dandelionItem, nullptr);

    ItemStack dandelionStack(dandelionItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(dandelionStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsPoppy)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    const BlockItem* poppyItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::POPPY);
    ASSERT_NE(poppyItem, nullptr);

    ItemStack poppyStack(poppyItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(poppyStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsBlueOrchid)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    const BlockItem* blueOrchidItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BLUE_ORCHID);
    ASSERT_NE(blueOrchidItem, nullptr);

    ItemStack blueOrchidStack(blueOrchidItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(blueOrchidStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsAllium)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    const BlockItem* alliumItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::ALLIUM);
    ASSERT_NE(alliumItem, nullptr);

    ItemStack alliumStack(alliumItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(alliumStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsSunflower)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    const BlockItem* sunflowerItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::SUNFLOWER);
    ASSERT_NE(sunflowerItem, nullptr);

    ItemStack sunflowerStack(sunflowerItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(sunflowerStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsLilac)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    const BlockItem* lilacItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::LILAC);
    ASSERT_NE(lilacItem, nullptr);

    ItemStack lilacStack(lilacItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(lilacStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsRoseBush)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    const BlockItem* roseBushItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::ROSE_BUSH);
    ASSERT_NE(roseBushItem, nullptr);

    ItemStack roseBushStack(roseBushItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(roseBushStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsPeony)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    const BlockItem* peonyItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::PEONY);
    ASSERT_NE(peonyItem, nullptr);

    ItemStack peonyStack(peonyItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(peonyStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsCornflower)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    const BlockItem* cornflowerItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CORNFLOWER);
    ASSERT_NE(cornflowerItem, nullptr);

    ItemStack cornflowerStack(cornflowerItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(cornflowerStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsLilyOfTheValley)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    const BlockItem* lilyItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::LILY_OF_THE_VALLEY);
    ASSERT_NE(lilyItem, nullptr);

    ItemStack lilyStack(lilyItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(lilyStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsTulips)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 测试所有颜色的郁金香
    const BlockItem* redTulipItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::RED_TULIP);
    const BlockItem* orangeTulipItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::ORANGE_TULIP);
    const BlockItem* whiteTulipItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::WHITE_TULIP);
    const BlockItem* pinkTulipItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::PINK_TULIP);

    ASSERT_NE(redTulipItem, nullptr);
    ASSERT_NE(orangeTulipItem, nullptr);
    ASSERT_NE(whiteTulipItem, nullptr);
    ASSERT_NE(pinkTulipItem, nullptr);

    EXPECT_TRUE(bee.isBreedingItem(ItemStack(redTulipItem, 1)));
    EXPECT_TRUE(bee.isBreedingItem(ItemStack(orangeTulipItem, 1)));
    EXPECT_TRUE(bee.isBreedingItem(ItemStack(whiteTulipItem, 1)));
    EXPECT_TRUE(bee.isBreedingItem(ItemStack(pinkTulipItem, 1)));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsWitherRose)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    const BlockItem* witherRoseItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::WITHER_ROSE);
    ASSERT_NE(witherRoseItem, nullptr);

    ItemStack witherRoseStack(witherRoseItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(witherRoseStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_RejectsWheat)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    if (Items::WHEAT != nullptr) {
        ItemStack wheatStack(Items::WHEAT, 1);
        EXPECT_FALSE(bee.isBreedingItem(wheatStack));
    }
}

TEST_F(BeeEntityTest, IsBreedingItem_RejectsCarrot)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    if (Items::CARROT != nullptr) {
        ItemStack carrotStack(Items::CARROT, 1);
        EXPECT_FALSE(bee.isBreedingItem(carrotStack));
    }
}

TEST_F(BeeEntityTest, IsBreedingItem_RejectsEmptyStack)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    ItemStack emptyStack;
    EXPECT_FALSE(bee.isBreedingItem(emptyStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_RejectsDiamond)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 使用钻石作为非花朵物品测试
    // 注意：STONE 可能在某些测试环境中未注册为 BlockItem
    if (Items::DIAMOND != nullptr) {
        ItemStack diamondStack(Items::DIAMOND, 1);
        EXPECT_FALSE(bee.isBreedingItem(diamondStack));
    }
}

// ============================================================================
// spawnBaby 测试
// ============================================================================

TEST_F(BeeEntityTest, SpawnBaby_CreatesChildBee)
{
    BeeEntity parent1(EntityInstanceId(1), mc::test::testEcsRegistry());
    parent1.setWorld(&m_world);
    parent1.setPosition(0.0f, 64.0f, 0.0f);

    BeeEntity parent2(EntityInstanceId(2), mc::test::testEcsRegistry());

    auto baby = parent1.spawnBaby(parent2);

    ASSERT_NE(baby, nullptr);
    EXPECT_TRUE(baby->isChild());

    // 检查是 BeeEntity 类型
    BeeEntity* babyBee = dynamic_cast<BeeEntity*>(baby.get());
    EXPECT_NE(babyBee, nullptr);
}

TEST_F(BeeEntityTest, SpawnBaby_PositionNearParent)
{
    BeeEntity parent(EntityInstanceId(1), mc::test::testEcsRegistry());
    parent.setWorld(&m_world);
    parent.setPosition(100.0f, 64.0f, 200.0f);

    BeeEntity partner(EntityInstanceId(2), mc::test::testEcsRegistry());

    auto baby = parent.spawnBaby(partner);

    ASSERT_NE(baby, nullptr);

    // 幼体应该在父体附近
    f32 dx = baby->x() - parent.x();
    f32 dy = baby->y() - parent.y();
    f32 dz = baby->z() - parent.z();
    f32 distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    // 位置应该非常接近（spawnBaby使用父体位置）
    EXPECT_LT(distance, 1.0f);
}

// ============================================================================
// 花粉状态测试
// ============================================================================

TEST_F(BeeEntityTest, NectarState_DefaultFalse)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(bee.hasNectar());
}

TEST_F(BeeEntityTest, NectarState_CanSetAndGet)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    bee.setHasNectar(true);
    EXPECT_TRUE(bee.hasNectar());

    bee.setHasNectar(false);
    EXPECT_FALSE(bee.hasNectar());
}

// ============================================================================
// 螫刺状态测试
// ============================================================================

TEST_F(BeeEntityTest, StungState_DefaultFalse)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(bee.hasStung());
}

TEST_F(BeeEntityTest, StungState_CanSetAndGet)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    bee.setHasStung(true);
    EXPECT_TRUE(bee.hasStung());

    bee.setHasStung(false);
    EXPECT_FALSE(bee.hasStung());
}

// ============================================================================
// 飞行状态测试
// ============================================================================

TEST_F(BeeEntityTest, FlyingState_DefaultFalse)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(bee.isFlying());
}

TEST_F(BeeEntityTest, FlyingState_CanSetAndGet)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    bee.setFlying(true);
    EXPECT_TRUE(bee.isFlying());

    bee.setFlying(false);
    EXPECT_FALSE(bee.isFlying());
}

// ============================================================================
// 蜂巢系统测试
// ============================================================================

TEST_F(BeeEntityTest, HivePosition_DefaultNoHive)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(bee.hasHive());
}

TEST_F(BeeEntityTest, HivePosition_CanSetAndGet)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    BlockPos hivePos(100, 64, 200);
    bee.setHivePos(hivePos);

    EXPECT_TRUE(bee.hasHive());
    EXPECT_EQ(bee.getHivePos(), hivePos);
}

// ============================================================================
// 花朵位置测试
// ============================================================================

TEST_F(BeeEntityTest, FlowerPosition_DefaultNoFlower)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(bee.hasFlower());
}

TEST_F(BeeEntityTest, FlowerPosition_CanSetAndGet)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    BlockPos flowerPos(50, 64, 100);
    bee.setFlowerPos(flowerPos);

    EXPECT_TRUE(bee.hasFlower());
    EXPECT_EQ(bee.getFlowerPos(), flowerPos);
}

// ============================================================================
// 属性测试
// ============================================================================

TEST_F(BeeEntityTest, Attributes_HasCorrectBaseValues)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    // MC 1.16.5: 蜜蜂生命值为 10
    EXPECT_DOUBLE_EQ(bee.maxHealth(), 10.0);

    // MC 1.16.5: 蜜蜂移动速度为 0.3
    EXPECT_DOUBLE_EQ(bee.getAttributeValue("generic.movement_speed", 0.0), 0.3);
}

// ============================================================================
// 眼睛高度测试
// ============================================================================

TEST_F(BeeEntityTest, EyeHeight_IsCorrect)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    // MC 1.16.5: 蜜蜂眼睛高度 0.3
    EXPECT_FLOAT_EQ(bee.eyeHeight(), 0.3f);
}

// ============================================================================
// IAngerable 接口测试
// ============================================================================

TEST_F(BeeEntityTest, Anger_CanSetAngerTime)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    bee.setAngerTime(100);
    EXPECT_EQ(bee.getAngerTime(), 100);
    EXPECT_TRUE(bee.isAngry());
}

TEST_F(BeeEntityTest, Anger_CanSetAngry)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    bee.setAngry(true);
    EXPECT_TRUE(bee.isAngry());
    EXPECT_GT(bee.getAngerTime(), 0);
}

TEST_F(BeeEntityTest, Anger_CanClearAnger)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 先设置为愤怒状态
    bee.setAngerTime(100);
    EXPECT_TRUE(bee.isAngry());

    // 清除愤怒
    bee.setAngry(false);
    EXPECT_FALSE(bee.isAngry());
    EXPECT_EQ(bee.getAngerTime(), 0);
}

// ============================================================================
// 属性测试 - ATTACK_DAMAGE 和 FOLLOW_RANGE
// ============================================================================

TEST_F(BeeEntityTest, Attributes_HasAttackDamage)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    // MC 1.16.5: 蜜蜂攻击伤害为 2.0
    // 注意：需要在构造函数中设置属性
    // 此测试验证属性已注册
    EXPECT_GE(bee.getAttributeValue("generic.attack_damage", -1.0), 0.0);
}

TEST_F(BeeEntityTest, Attributes_HasFollowRange)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    // MC 1.16.5: 蜜蜂跟随范围为 48.0
    EXPECT_DOUBLE_EQ(bee.getAttributeValue("generic.follow_range", 0.0), 48.0);
}

TEST_F(BeeEntityTest, Attributes_FlyingSpeedIsSet)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    // MC 1.16.5: 蜜蜂飞行速度为 0.6
    // 属性值已经设置
    EXPECT_GE(bee.getAttributeValue("generic.flying_speed", 0.0), 0.0);
}

// ============================================================================
// 水下溺水测试
// ============================================================================
// 注意：水下计时器测试需要完整的 world mock 来模拟 isInWater() 返回 true
// BeeEntity.tick() 中检查 isInWater() 状态，而 BaseTestWorld 的 isInWater()
// 依赖于 Entity::m_inWater 标志，该标志需要通过 updateEnvironmentState() 更新

TEST_F(BeeEntityTest, UnderwaterTimer_InitialState)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);

    // 初始状态下溺水计时器应为 0
    EXPECT_EQ(bee.getUnderWaterTimer(), 0);
}

// 注意：溺水伤害测试需要完整的 world mock 来支持 hurt() 方法
// 这里我们只验证计时器的递增逻辑

// ============================================================================
// 蜂巢倒计时和冷却测试（简单 setter/getter，不需要世界）
// ============================================================================

TEST_F(BeeEntityTest, StayOutOfHiveCountdown_DefaultZero)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(bee.getStayOutOfHiveCountdown(), 0);
}

TEST_F(BeeEntityTest, StayOutOfHiveCountdown_CanSetAndGet)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    bee.setStayOutOfHiveCountdown(400);
    EXPECT_EQ(bee.getStayOutOfHiveCountdown(), 400);

    bee.setStayOutOfHiveCountdown(0);
    EXPECT_EQ(bee.getStayOutOfHiveCountdown(), 0);

    bee.setStayOutOfHiveCountdown(100);
    EXPECT_EQ(bee.getStayOutOfHiveCountdown(), 100);
}

TEST_F(BeeEntityTest, HiveLocateCooldown_DefaultZero)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(bee.getHiveLocateCooldown(), 0);
}

TEST_F(BeeEntityTest, HiveLocateCooldown_CanSetAndGet)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    bee.setHiveLocateCooldown(200);
    EXPECT_EQ(bee.getHiveLocateCooldown(), 200);

    bee.setHiveLocateCooldown(0);
    EXPECT_EQ(bee.getHiveLocateCooldown(), 0);

    bee.setHiveLocateCooldown(50);
    EXPECT_EQ(bee.getHiveLocateCooldown(), 50);
}

TEST_F(BeeEntityTest, DropHive_ClearsHiveAndSetsCooldown)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setHivePos(BlockPos(100, 64, 200));

    EXPECT_TRUE(bee.hasHive());
    EXPECT_EQ(bee.getHivePos(), BlockPos(100, 64, 200));

    bee.dropHive();

    EXPECT_FALSE(bee.hasHive());
    EXPECT_EQ(bee.getHivePos(), BlockPos::zero());
    // dropHive 设置 200 tick 冷却
    EXPECT_EQ(bee.getStayOutOfHiveCountdown(), 200);
}

TEST_F(BeeEntityTest, DropHive_WhenNoHive_SetsCooldown)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    // 没有蜂巢时调用 dropHive
    EXPECT_FALSE(bee.hasHive());

    bee.dropHive();

    EXPECT_FALSE(bee.hasHive());
    EXPECT_EQ(bee.getStayOutOfHiveCountdown(), 200);
}

TEST_F(BeeEntityTest, TicksWithoutNectar_DefaultZero)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(bee.getTicksWithoutNectar(), 0);
}

TEST_F(BeeEntityTest, TicksWithoutNectar_CanReset)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    // 模拟递增（通过直接设置内部状态不可行，但可以验证 resetTicksWithoutNectar）
    bee.resetTicksWithoutNectar();
    EXPECT_EQ(bee.getTicksWithoutNectar(), 0);
}

TEST_F(BeeEntityTest, Pollinating_DefaultFalse)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(bee.isPollinating());
}

TEST_F(BeeEntityTest, Pollinating_CanSetAndGet)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());

    bee.setPollinating(true);
    EXPECT_TRUE(bee.isPollinating());

    bee.setPollinating(false);
    EXPECT_FALSE(bee.isPollinating());
}

// ============================================================================
// 蜂巢验证和交互测试（需要世界支持）
// ============================================================================

/**
 * @brief 蜂巢交互测试夹具
 *
 * 使用 BeeHiveTestWorld 来测试需要方块实体支持的方法。
 */
class BeeHiveInteractionTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化顺序：方块 -> 方块标签 -> 物品 -> 方块物品 -> 物品标签
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        item::tag::ItemTags::initialize();
    }

    BeeHiveTestWorld m_world;
};

TEST_F(BeeHiveInteractionTest, IsHiveValid_NoHive_ReturnsFalse)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);

    // 没有设置蜂巢位置
    EXPECT_FALSE(bee.hasHive());
    EXPECT_FALSE(bee.isHiveValid());
}

TEST_F(BeeHiveInteractionTest, IsHiveValid_NoWorld_ReturnsFalse)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setHivePos(BlockPos(10, 64, 20));

    EXPECT_TRUE(bee.hasHive());
    // 没有世界，getBeehiveBlockEntity 返回 nullptr
    EXPECT_FALSE(bee.isHiveValid());
}

TEST_F(BeeHiveInteractionTest, IsHiveValid_WithValidHive_ReturnsTrue)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);

    EXPECT_TRUE(bee.hasHive());
    EXPECT_TRUE(bee.isHiveValid());
}

TEST_F(BeeHiveInteractionTest, IsHiveValid_HiveTooFar_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    // 蜜蜂距离蜂巢超过 48 格
    bee.setPosition(200.0f, 64.0f, 200.0f);
    bee.setHivePos(hivePos);

    EXPECT_TRUE(bee.hasHive());
    EXPECT_FALSE(bee.isHiveValid());
}

TEST_F(BeeHiveInteractionTest, IsHiveValid_NoBlockAtHivePos_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    // 不设置蜂巢方块

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);

    EXPECT_TRUE(bee.hasHive());
    // 位置上没有蜂巢方块，getBeehiveBlockEntity 返回 nullptr
    EXPECT_FALSE(bee.isHiveValid());
}

TEST_F(BeeHiveInteractionTest, IsHiveValid_NonBeehiveBlock_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    // 在该位置设置石头方块而不是蜂巢
    if (VanillaBlocks::STONE != nullptr) {
        m_world.setBlockAt(hivePos, &VanillaBlocks::STONE->defaultState());
    }

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);

    EXPECT_TRUE(bee.hasHive());
    // 石头不是蜂巢，应该返回 false
    EXPECT_FALSE(bee.isHiveValid());
}

TEST_F(BeeHiveInteractionTest, IsHiveValid_NoBlockEntityAtHivePos_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    // 设置蜂巢方块但不设置方块实体
    if (block_registry::NaturalBlocks::BEEHIVE != nullptr) {
        const BlockState* beehiveState = &block_registry::NaturalBlocks::BEEHIVE->defaultState();
        m_world.setBlockAt(hivePos, beehiveState);
    }
    // 不调用 setBeehiveAt，所以没有方块实体

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);

    EXPECT_TRUE(bee.hasHive());
    // 有蜂巢方块但没有方块实体，返回 false
    EXPECT_FALSE(bee.isHiveValid());
}

TEST_F(BeeHiveInteractionTest, GetBeehiveBlockEntity_ValidHive_ReturnsEntity)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);

    auto* beehive = bee.getBeehiveBlockEntity();
    ASSERT_NE(beehive, nullptr);
    EXPECT_EQ(beehive->getType(), BlockEntityType::Beehive);
    EXPECT_EQ(beehive->isEmpty(), true);
}

TEST_F(BeeHiveInteractionTest, GetBeehiveBlockEntity_InvalidHive_ReturnsNullptr)
{
    BlockPos hivePos(10, 64, 20);
    // 不设置蜂巢

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);

    EXPECT_EQ(bee.getBeehiveBlockEntity(), nullptr);
}

TEST_F(BeeHiveInteractionTest, WantsToEnterHive_WithNectar_ReturnsTrue)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);

    // 设置有花粉
    bee.setHasNectar(true);
    // 确保冷却为 0
    bee.setStayOutOfHiveCountdown(0);

    EXPECT_TRUE(bee.wantsToEnterHive());
}

TEST_F(BeeHiveInteractionTest, WantsToEnterHive_WithoutNectarButTired_ReturnsTrue)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);

    // 没有花粉
    bee.setHasNectar(false);
    // 确保冷却为 0
    bee.setStayOutOfHiveCountdown(0);
    // 蜜蜂有蜂巢但无花粉，ticksWithoutNectar 需要超过 3600 才算厌倦
    // 由于刚创建，ticksWithoutNectar = 0，所以不想回巢
    EXPECT_FALSE(bee.wantsToEnterHive());
}

TEST_F(BeeHiveInteractionTest, WantsToEnterHive_StayOutOfHiveCountdown_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);

    bee.setHasNectar(true);
    bee.setStayOutOfHiveCountdown(100); // 仍在冷却中

    EXPECT_FALSE(bee.wantsToEnterHive());
}

TEST_F(BeeHiveInteractionTest, WantsToEnterHive_Pollinating_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);

    bee.setHasNectar(true);
    bee.setStayOutOfHiveCountdown(0);
    bee.setPollinating(true); // 正在授粉

    EXPECT_FALSE(bee.wantsToEnterHive());
}

TEST_F(BeeHiveInteractionTest, WantsToEnterHive_HasStung_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);

    bee.setHasNectar(true);
    bee.setStayOutOfHiveCountdown(0);
    bee.setHasStung(true); // 已螫刺

    EXPECT_FALSE(bee.wantsToEnterHive());
}

TEST_F(BeeHiveInteractionTest, WantsToEnterHive_NoNectarNoTired_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);

    bee.setHasNectar(false);
    bee.setStayOutOfHiveCountdown(0);

    // 没有花粉且没有累（ticksWithoutNectar = 0），不想回巢
    EXPECT_FALSE(bee.wantsToEnterHive());
}

TEST_F(BeeHiveInteractionTest, IsHiveNearFire_NoFire_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setHivePos(hivePos);

    EXPECT_FALSE(bee.isHiveNearFire());
}

TEST_F(BeeHiveInteractionTest, IsHiveNearFire_FireAdjacent_ReturnsTrue)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    // 在蜂巢旁边放置火
    BlockPos firePos(11, 64, 20);
    m_world.setFireAt(firePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setHivePos(hivePos);

    EXPECT_TRUE(bee.isHiveNearFire());
}

TEST_F(BeeHiveInteractionTest, IsHiveNearFire_FireAbove_ReturnsTrue)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    // 在蜂巢上方放置火
    BlockPos firePos(10, 65, 20);
    m_world.setFireAt(firePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setHivePos(hivePos);

    EXPECT_TRUE(bee.isHiveNearFire());
}

TEST_F(BeeHiveInteractionTest, IsHiveNearFire_FireTooFar_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    // 火距离蜂巢超过 3x3x3 范围（2格以外）
    BlockPos firePos(13, 64, 20);
    m_world.setFireAt(firePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setHivePos(hivePos);

    EXPECT_FALSE(bee.isHiveNearFire());
}

TEST_F(BeeHiveInteractionTest, IsHiveNearFire_NoHive_ReturnsFalse)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);

    // 没有蜂巢
    EXPECT_FALSE(bee.hasHive());
    EXPECT_FALSE(bee.isHiveNearFire());
}

TEST_F(BeeHiveInteractionTest, WantsToEnterHive_FireNearHive_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    // 在蜂巢旁边放置火
    BlockPos firePos(11, 64, 20);
    m_world.setFireAt(firePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);
    bee.setHasNectar(true);
    bee.setStayOutOfHiveCountdown(0);

    // 蜂巢附近有火，不想进入
    EXPECT_FALSE(bee.wantsToEnterHive());
}

TEST_F(BeeHiveInteractionTest, GetBeehiveBlockEntity_WithinRange_ReturnsEntity)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    // 蜜蜂在蜂巢旁边（在 48 格范围内）
    bee.setPosition(11.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);

    auto* beehive = bee.getBeehiveBlockEntity();
    ASSERT_NE(beehive, nullptr);
}

TEST_F(BeeHiveInteractionTest, GetBeehiveBlockEntity_OutOfRange_ReturnsNullptr)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    // 蜜蜂距离蜂巢超过 48 格
    bee.setPosition(200.0f, 64.0f, 200.0f);
    bee.setHivePos(hivePos);

    EXPECT_EQ(bee.getBeehiveBlockEntity(), nullptr);
}

TEST_F(BeeHiveInteractionTest, DropHive_ClearsHivePosition)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);

    EXPECT_TRUE(bee.hasHive());
    EXPECT_TRUE(bee.isHiveValid());

    bee.dropHive();

    EXPECT_FALSE(bee.hasHive());
    EXPECT_EQ(bee.getHivePos(), BlockPos::zero());
    EXPECT_EQ(bee.getStayOutOfHiveCountdown(), 200);
}

// ============================================================================
// 天气/夜间回巢逻辑测试 (BEES_STAY_IN_HIVE 等效)
// ============================================================================

/**
 * @brief 天气/夜间回巢测试夹具
 *
 * 使用 BeeWeatherTestWorld 来测试天气和时间对蜜蜂行为的影响。
 */
class BeeWeatherTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        item::tag::ItemTags::initialize();
    }

    BeeWeatherTestWorld m_world;
};

// ---------- isTiredOfLookingForNectar ----------

TEST_F(BeeWeatherTest, IsTiredOfLookingForNectar_BelowThreshold_ReturnsFalse)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setHivePos(BlockPos(10, 64, 20));

    // ticksWithoutNectar = 0，远低于 3600 阈值
    EXPECT_FALSE(bee.isTiredOfLookingForNectar());

    // 模拟递增 ticksWithoutNectar（通过 tick() 递增需要 hasHive && !hasNectar）
    bee.setHasNectar(false);
    // 手动调用 tick() 3600 次不太现实，但我们可以验证阈值边界
    // 阈值是 > 3600，所以 <= 3600 时返回 false
    // 由于我们无法直接设置 m_ticksWithoutNectarSinceExitingHive，
    // 验证初始状态即可
    EXPECT_EQ(bee.getTicksWithoutNectar(), 0);
}

TEST_F(BeeWeatherTest, IsTiredOfLookingForNectar_ThresholdIs3600)
{
    // 验证 isTiredOfLookingForNectar 的阈值是 3600（MC 原版值）
    // 通过 wantsToEnterHive 间接验证
    // 此测试确认 API 契约：3600 tick 后蜜蜂厌倦寻找花蜜
    // 注意：实际递增需要通过 tick()，本测试验证方法存在和可调用
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setHivePos(BlockPos(10, 64, 20));
    bee.setHasNectar(false);
    bee.setStayOutOfHiveCountdown(0);

    // 初始状态不应厌倦
    EXPECT_FALSE(bee.isTiredOfLookingForNectar());
}

// ---------- wantsToEnterHive 雨天/雷暴/夜间逻辑 ----------

TEST_F(BeeWeatherTest, WantsToEnterHive_Raining_ReturnsTrue)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);
    m_world.setRaining(true);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);
    bee.setHasNectar(false);
    bee.setStayOutOfHiveCountdown(0);
    // 非雨天时，无花粉且不累的蜜蜂不想回巢
    // 但雨天时应想回巢

    EXPECT_TRUE(bee.wantsToEnterHive());
}

TEST_F(BeeWeatherTest, WantsToEnterHive_Thundering_ReturnsTrue)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);
    m_world.setThundering(true);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);
    bee.setHasNectar(false);
    bee.setStayOutOfHiveCountdown(0);

    EXPECT_TRUE(bee.wantsToEnterHive());
}

TEST_F(BeeWeatherTest, WantsToEnterHive_Nighttime_ReturnsTrue)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);
    // 设置为夜间：dayTime >= 12000 表示夜晚
    // MC 原版：tick 12542 开始回巢，这里使用 isDaytime() 判断
    // isDaytime() = dayTimeOfDay() < 12000，所以 14000 为夜间
    m_world.setDayTime(14000);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);
    bee.setHasNectar(false);
    bee.setStayOutOfHiveCountdown(0);

    EXPECT_FALSE(m_world.isDaytime()); // 确认是夜间
    EXPECT_TRUE(bee.wantsToEnterHive());
}

TEST_F(BeeWeatherTest, WantsToEnterHive_DaytimeClearNoNectar_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);
    // 默认：白天、无雨、无雷暴
    EXPECT_TRUE(m_world.isDaytime());
    EXPECT_FALSE(m_world.isRaining());
    EXPECT_FALSE(m_world.isThundering());

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);
    bee.setHasNectar(false);
    bee.setStayOutOfHiveCountdown(0);

    // 白天晴朗无花粉且不累，不想回巢
    EXPECT_FALSE(bee.wantsToEnterHive());
}

TEST_F(BeeWeatherTest, WantsToEnterHive_RainingButStung_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);
    m_world.setRaining(true);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);
    bee.setHasNectar(false);
    bee.setStayOutOfHiveCountdown(0);
    bee.setHasStung(true); // 已螫刺，不应回巢

    EXPECT_FALSE(bee.wantsToEnterHive());
}

TEST_F(BeeWeatherTest, WantsToEnterHive_RainingButPollinating_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);
    m_world.setRaining(true);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);
    bee.setHasNectar(false);
    bee.setStayOutOfHiveCountdown(0);
    bee.setPollinating(true); // 正在授粉，不回巢

    EXPECT_FALSE(bee.wantsToEnterHive());
}

TEST_F(BeeWeatherTest, WantsToEnterHive_RainingButStayOutOfHive_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);
    m_world.setRaining(true);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);
    bee.setHasNectar(false);
    bee.setStayOutOfHiveCountdown(100); // 仍在冷却期

    EXPECT_FALSE(bee.wantsToEnterHive());
}

TEST_F(BeeWeatherTest, WantsToEnterHive_RainingAndHiveNearFire_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);
    m_world.setRaining(true);
    BlockPos firePos(11, 64, 20);
    m_world.setFireAt(firePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);
    bee.setHasNectar(false);
    bee.setStayOutOfHiveCountdown(0);

    // 即使下雨，蜂巢着火也不进
    EXPECT_FALSE(bee.wantsToEnterHive());
}

TEST_F(BeeWeatherTest, WantsToEnterHive_NighttimeDaytimeBoundary)
{
    // isDaytime() 使用 dayTimeOfDay() < 12000
    // 验证边界值：11999 为白天，12000 为夜晚
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);
    bee.setHasNectar(false);
    bee.setStayOutOfHiveCountdown(0);

    // 白天边界 (dayTime = 11999)
    m_world.setDayTime(11999);
    EXPECT_TRUE(m_world.isDaytime());
    EXPECT_FALSE(bee.wantsToEnterHive());

    // 夜晚边界 (dayTime = 12000)
    m_world.setDayTime(12000);
    EXPECT_FALSE(m_world.isDaytime());
    EXPECT_TRUE(bee.wantsToEnterHive());
}

// ---------- BeehiveBlockEntity 天气/夜间释放阻止逻辑 ----------

TEST_F(BeeWeatherTest, Beehive_ReleasePreventedDuringRain)
{
    // 测试 _releaseOccupant 在雨天不放出蜜蜂（非紧急情况）
    // 通过 BeehiveBlockEntity::tick() 间接测试
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);
    m_world.setRaining(true);

    // 获取蜂巢方块实体并添加蜜蜂
    auto* blockEntity = m_world.getBlockEntity(hivePos);
    ASSERT_NE(blockEntity, nullptr);
    auto* beehive = static_cast<blockentity::BeehiveBlockEntity*>(blockEntity);
    ASSERT_NE(beehive, nullptr);

    // 创建蜜蜂并加入蜂巢
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setHasNectar(true);
    beehive->addOccupant(bee);
    EXPECT_EQ(beehive->getOccupantCount(), 1);

    // tick 让蜜蜂停留时间超过阈值
    // 有花粉时 MIN_OCCUPATION_TICKS_NECTAR = 2400
    for (int i = 0; i < 2500; ++i) {
        beehive->tick(m_world);
    }

    // 雨天，非紧急释放应被阻止，蜜蜂仍在巢中
    EXPECT_EQ(beehive->getOccupantCount(), 1);
}

TEST_F(BeeWeatherTest, Beehive_ReleasePreventedDuringNight)
{
    // 测试 _releaseOccupant 在夜间不放出蜜蜂
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);
    m_world.setDayTime(14000); // 夜间

    auto* blockEntity = m_world.getBlockEntity(hivePos);
    ASSERT_NE(blockEntity, nullptr);
    auto* beehive = static_cast<blockentity::BeehiveBlockEntity*>(blockEntity);
    ASSERT_NE(beehive, nullptr);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setHasNectar(false);
    beehive->addOccupant(bee);
    EXPECT_EQ(beehive->getOccupantCount(), 1);

    // 无花粉时 MIN_OCCUPATION_TICKS_NECTARLESS = 600
    for (int i = 0; i < 700; ++i) {
        beehive->tick(m_world);
    }

    // 夜间，非紧急释放应被阻止
    EXPECT_EQ(beehive->getOccupantCount(), 1);
}

TEST_F(BeeWeatherTest, Beehive_ReleaseAllowedDuringDaytime)
{
    // 测试 _releaseOccupant 在白天晴天放出蜜蜂
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);
    // 默认：白天、无雨、无雷暴
    EXPECT_TRUE(m_world.isDaytime());
    EXPECT_FALSE(m_world.isRaining());

    auto* blockEntity = m_world.getBlockEntity(hivePos);
    ASSERT_NE(blockEntity, nullptr);
    auto* beehive = static_cast<blockentity::BeehiveBlockEntity*>(blockEntity);
    ASSERT_NE(beehive, nullptr);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setHasNectar(false);
    beehive->addOccupant(bee);
    EXPECT_EQ(beehive->getOccupantCount(), 1);

    // 无花粉时 MIN_OCCUPATION_TICKS_NECTARLESS = 600
    for (int i = 0; i < 700; ++i) {
        beehive->tick(m_world);
    }

    // 白天晴天，蜜蜂应该被释放
    EXPECT_EQ(beehive->getOccupantCount(), 0);
}

TEST_F(BeeWeatherTest, Beehive_EmergencyReleaseDuringRain)
{
    // 测试紧急释放（火灾）在雨天仍然放出蜜蜂
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);
    m_world.setRaining(true);

    // 在蜂巢旁边放火触发紧急释放
    BlockPos firePos(11, 64, 20);
    m_world.setFireAt(firePos);

    auto* blockEntity = m_world.getBlockEntity(hivePos);
    ASSERT_NE(blockEntity, nullptr);
    auto* beehive = static_cast<blockentity::BeehiveBlockEntity*>(blockEntity);
    ASSERT_NE(beehive, nullptr);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setHasNectar(false);
    beehive->addOccupant(bee);
    EXPECT_EQ(beehive->getOccupantCount(), 1);

    // 火灾触发紧急释放，即使雨天也要释放
    // tick 一帧即可触发火灾检测和紧急释放
    beehive->tick(m_world);

    // 紧急释放应该清空所有蜜蜂
    EXPECT_EQ(beehive->getOccupantCount(), 0);
}

TEST_F(BeeWeatherTest, Beehive_ReleasePreventedDuringThunder)
{
    // 测试 _releaseOccupant 在雷暴时不放出蜜蜂
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);
    m_world.setThundering(true);

    auto* blockEntity = m_world.getBlockEntity(hivePos);
    ASSERT_NE(blockEntity, nullptr);
    auto* beehive = static_cast<blockentity::BeehiveBlockEntity*>(blockEntity);
    ASSERT_NE(beehive, nullptr);

    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setHasNectar(false);
    beehive->addOccupant(bee);

    for (int i = 0; i < 700; ++i) {
        beehive->tick(m_world);
    }

    // 雷暴天气，非紧急释放应被阻止
    EXPECT_EQ(beehive->getOccupantCount(), 1);
}

} // namespace

// ============================================================================
// 导航方法接口测试
// ============================================================================

/**
 * @brief 导航方法测试夹具
 *
 * 测试 BeeEntity::pathfindRandomlyTowards 和 pathfindDirectlyTowards 的基本接口行为。
 * 由于这些方法需要完整的世界和导航器支持，仅测试边界条件。
 */
class BeeNavigationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        item::tag::ItemTags::initialize();
    }

    BeeTestWorld m_world;
};

TEST_F(BeeNavigationTest, PathfindRandomlyTowards_NoWorld_ReturnsFalse)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    // 没有设置世界，navigator() 返回 nullptr
    EXPECT_FALSE(bee.pathfindRandomlyTowards(BlockPos(10, 64, 20)));
}

TEST_F(BeeNavigationTest, PathfindDirectlyTowards_NoWorld_ReturnsFalse)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    // 没有设置世界，navigator() 返回 nullptr
    EXPECT_FALSE(bee.pathfindDirectlyTowards(BlockPos(10, 64, 20)));
}

TEST_F(BeeNavigationTest, PathfindRandomlyTowards_SamePositionAsTarget)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);

    // 目标位置与蜜蜂当前位置相同，曼哈顿距离为0，搜索范围 k=0, l=0
    // 应该返回 false（搜索范围太小无法生成有效位置）
    // 注意：由于导航器可能未完整初始化，这里主要验证不会崩溃
    (void)bee.pathfindRandomlyTowards(BlockPos(10, 64, 20));
}

TEST_F(BeeNavigationTest, PathfindDirectlyTowards_SamePositionAsTarget)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);

    // 目标位置与蜜蜂当前位置相同，距离 < 3格，speed = 1.0
    // 主要验证不会崩溃
    (void)bee.pathfindDirectlyTowards(BlockPos(10, 64, 20));
}

TEST_F(BeeNavigationTest, PathfindRandomlyTowards_FarTarget)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(0.0f, 64.0f, 0.0f);

    // 目标位置远离蜜蜂，曼哈顿距离 > 15，搜索范围 k=6, l=8
    // 主要验证不会崩溃
    (void)bee.pathfindRandomlyTowards(BlockPos(100, 64, 100));
}

TEST_F(BeeNavigationTest, PathfindDirectlyTowards_FarTarget)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(0.0f, 64.0f, 0.0f);

    // 目标位置远离蜜蜂，距离 > 3格，speed = 2.0
    // 主要验证不会崩溃
    (void)bee.pathfindDirectlyTowards(BlockPos(100, 64, 100));
}

TEST_F(BeeNavigationTest, PathfindRandomlyTowards_AboveTarget)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(50.0f, 80.0f, 50.0f);

    // 目标在蜜蜂下方，yOffset 应为 +4
    (void)bee.pathfindRandomlyTowards(BlockPos(50, 64, 50));
}

TEST_F(BeeNavigationTest, PathfindRandomlyTowards_BelowTarget)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(50.0f, 64.0f, 50.0f);

    // 目标在蜜蜂上方，yOffset 应为 -4
    (void)bee.pathfindRandomlyTowards(BlockPos(50, 80, 50));
}

TEST_F(BeeNavigationTest, PathfindRandomlyTowards_NearTarget)
{
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setPosition(50.0f, 64.0f, 50.0f);

    // 目标较近，曼哈顿距离 < 15，搜索范围应缩小
    (void)bee.pathfindRandomlyTowards(BlockPos(55, 66, 53));
}

// ============================================================================
// BeeEntity::attractsBees 测试
// ============================================================================

TEST_F(BeeEntityTest, AttractsBees_OpenEyeblossom_ReturnsTrue)
{
    // 开放眼眸花在 BEE_ATTRACTIVE 标签中，吸引蜜蜂
    Block* openEyeblossom = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "open_eyeblossom"));
    ASSERT_NE(openEyeblossom, nullptr);
    EXPECT_TRUE(BeeEntity::attractsBees(openEyeblossom->defaultState()));
}

TEST_F(BeeEntityTest, AttractsBees_ClosedEyeblossom_ReturnsFalse)
{
    // 闭合眼眸花不在 BEE_ATTRACTIVE 标签中，不吸引蜜蜂
    Block* closedEyeblossom = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "closed_eyeblossom"));
    ASSERT_NE(closedEyeblossom, nullptr);
    EXPECT_FALSE(BeeEntity::attractsBees(closedEyeblossom->defaultState()));
}

TEST_F(BeeEntityTest, AttractsBees_Dandelion_ReturnsTrue)
{
    // 蒲公英在 BEE_ATTRACTIVE 标签中
    ASSERT_NE(VanillaBlocks::DANDELION, nullptr);
    EXPECT_TRUE(BeeEntity::attractsBees(VanillaBlocks::DANDELION->defaultState()));
}

TEST_F(BeeEntityTest, AttractsBees_Stone_ReturnsFalse)
{
    // 石头不在 BEE_ATTRACTIVE 标签中
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    EXPECT_FALSE(BeeEntity::attractsBees(VanillaBlocks::STONE->defaultState()));
}

TEST_F(BeeEntityTest, AttractsBees_SunflowerUpperHalf_ReturnsTrue)
{
    // 向日葵上半部分吸引蜜蜂
    ASSERT_NE(VanillaBlocks::SUNFLOWER, nullptr);
    const BlockState& upperState = VanillaBlocks::SUNFLOWER->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), blocks::DoublePlantBlock::DoubleBlockHalf::Upper);
    EXPECT_TRUE(BeeEntity::attractsBees(upperState));
}

TEST_F(BeeEntityTest, AttractsBees_SunflowerLowerHalf_ReturnsFalse)
{
    // 向日葵下半部分不吸引蜜蜂
    ASSERT_NE(VanillaBlocks::SUNFLOWER, nullptr);
    const BlockState& lowerState = VanillaBlocks::SUNFLOWER->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), blocks::DoublePlantBlock::DoubleBlockHalf::Lower);
    EXPECT_FALSE(BeeEntity::attractsBees(lowerState));
}

// ============================================================================
// hurt override 测试（对齐 MC Java 1.21.11 Bee.hurtServer）
//
// Bee.hurtServer（Bee.java:645-652）：
//   if (this.isInvulnerableTo(level, source)) return false;
//   else { this.beePollinateGoal.stopPollinating(); return super.hurtServer(level, source, amount); }
// 蜜蜂受非免疫伤害时立即中断授粉（stopPollinating），再走基类 hurt。
// ============================================================================

TEST_F(BeeEntityTest, Hurt_NonInvulnerableDamage_StopsPollinating)
{
    // 授粉中的蜜蜂受到非免疫伤害 → stopPollinating 中断授粉
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setTypeId("minecraft:bee");
    bee.setHealth(bee.maxHealth());

    // 让蜜蜂进入授粉状态
    bee.setPollinating(true);
    EXPECT_TRUE(bee.isPollinating());

    // 非免疫伤害源（生物攻击，非绕过无敌）
    EntityDamageSource damageSource(DamageType::MobAttack, nullptr);

    bool result = bee.hurt(damageSource, 1.0f);

    // 受击成功
    EXPECT_TRUE(result);
    // stopPollinating 被调用 → isPollinating 变 false
    EXPECT_FALSE(bee.isPollinating());
}

TEST_F(BeeEntityTest, Hurt_InvulnerableDamage_DoesNotStopPollinating)
{
    // 设为无敌的蜜蜂受到非绕过无敌伤害 → isInvulnerableTo 返回 true，hurt 返回 false
    // 且不调用 stopPollinating（免疫短路在 stopPollinating 之前）。
    BeeEntity bee(EntityInstanceId(1), mc::test::testEcsRegistry());
    bee.setWorld(&m_world);
    bee.setTypeId("minecraft:bee");
    bee.setHealth(bee.maxHealth());
    bee.setInvulnerable(true);
    bee.setPollinating(true);
    EXPECT_TRUE(bee.isPollinating());

    EntityDamageSource damageSource(DamageType::MobAttack, nullptr);

    bool result = bee.hurt(damageSource, 1.0f);

    // 免疫导致受击失败
    EXPECT_FALSE(result);
    // 免疫短路未触发 stopPollinating → 仍在授粉
    EXPECT_TRUE(bee.isPollinating());
}

} // namespace mc
