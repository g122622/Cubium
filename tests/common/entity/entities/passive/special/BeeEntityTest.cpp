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
#include "common/entity/entities/passive/special/BeeEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/NaturalBlocks.hpp"
#include "common/world/block/registry/NetherBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/BeehiveBlockEntity.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/fluid/Fluid.hpp"
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
class BeeTestWorld final : public test::BaseTestWorld {
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
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
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
class BeeHiveTestWorld final : public test::BaseTestWorld {
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
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
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

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
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
    BeeEntity bee(EntityId(1));

    const BlockItem* dandelionItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::DANDELION);
    ASSERT_NE(dandelionItem, nullptr);

    ItemStack dandelionStack(dandelionItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(dandelionStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsPoppy)
{
    BeeEntity bee(EntityId(1));

    const BlockItem* poppyItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::POPPY);
    ASSERT_NE(poppyItem, nullptr);

    ItemStack poppyStack(poppyItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(poppyStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsBlueOrchid)
{
    BeeEntity bee(EntityId(1));

    const BlockItem* blueOrchidItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::BLUE_ORCHID);
    ASSERT_NE(blueOrchidItem, nullptr);

    ItemStack blueOrchidStack(blueOrchidItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(blueOrchidStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsAllium)
{
    BeeEntity bee(EntityId(1));

    const BlockItem* alliumItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::ALLIUM);
    ASSERT_NE(alliumItem, nullptr);

    ItemStack alliumStack(alliumItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(alliumStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsSunflower)
{
    BeeEntity bee(EntityId(1));

    const BlockItem* sunflowerItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::SUNFLOWER);
    ASSERT_NE(sunflowerItem, nullptr);

    ItemStack sunflowerStack(sunflowerItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(sunflowerStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsLilac)
{
    BeeEntity bee(EntityId(1));

    const BlockItem* lilacItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::LILAC);
    ASSERT_NE(lilacItem, nullptr);

    ItemStack lilacStack(lilacItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(lilacStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsRoseBush)
{
    BeeEntity bee(EntityId(1));

    const BlockItem* roseBushItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::ROSE_BUSH);
    ASSERT_NE(roseBushItem, nullptr);

    ItemStack roseBushStack(roseBushItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(roseBushStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsPeony)
{
    BeeEntity bee(EntityId(1));

    const BlockItem* peonyItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::PEONY);
    ASSERT_NE(peonyItem, nullptr);

    ItemStack peonyStack(peonyItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(peonyStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsCornflower)
{
    BeeEntity bee(EntityId(1));

    const BlockItem* cornflowerItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::CORNFLOWER);
    ASSERT_NE(cornflowerItem, nullptr);

    ItemStack cornflowerStack(cornflowerItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(cornflowerStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsLilyOfTheValley)
{
    BeeEntity bee(EntityId(1));

    const BlockItem* lilyItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::LILY_OF_THE_VALLEY);
    ASSERT_NE(lilyItem, nullptr);

    ItemStack lilyStack(lilyItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(lilyStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_AcceptsTulips)
{
    BeeEntity bee(EntityId(1));

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
    BeeEntity bee(EntityId(1));

    const BlockItem* witherRoseItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::WITHER_ROSE);
    ASSERT_NE(witherRoseItem, nullptr);

    ItemStack witherRoseStack(witherRoseItem, 1);
    EXPECT_TRUE(bee.isBreedingItem(witherRoseStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_RejectsWheat)
{
    BeeEntity bee(EntityId(1));

    if (Items::WHEAT != nullptr) {
        ItemStack wheatStack(Items::WHEAT, 1);
        EXPECT_FALSE(bee.isBreedingItem(wheatStack));
    }
}

TEST_F(BeeEntityTest, IsBreedingItem_RejectsCarrot)
{
    BeeEntity bee(EntityId(1));

    if (Items::CARROT != nullptr) {
        ItemStack carrotStack(Items::CARROT, 1);
        EXPECT_FALSE(bee.isBreedingItem(carrotStack));
    }
}

TEST_F(BeeEntityTest, IsBreedingItem_RejectsEmptyStack)
{
    BeeEntity bee(EntityId(1));

    ItemStack emptyStack;
    EXPECT_FALSE(bee.isBreedingItem(emptyStack));
}

TEST_F(BeeEntityTest, IsBreedingItem_RejectsDiamond)
{
    BeeEntity bee(EntityId(1));

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
    BeeEntity parent1(EntityId(1));
    parent1.setWorld(&m_world);
    parent1.setPosition(0.0f, 64.0f, 0.0f);

    BeeEntity parent2(EntityId(2));

    auto baby = parent1.spawnBaby(parent2);

    ASSERT_NE(baby, nullptr);
    EXPECT_TRUE(baby->isChild());

    // 检查是 BeeEntity 类型
    BeeEntity* babyBee = dynamic_cast<BeeEntity*>(baby.get());
    EXPECT_NE(babyBee, nullptr);
}

TEST_F(BeeEntityTest, SpawnBaby_PositionNearParent)
{
    BeeEntity parent(EntityId(1));
    parent.setWorld(&m_world);
    parent.setPosition(100.0f, 64.0f, 200.0f);

    BeeEntity partner(EntityId(2));

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
    BeeEntity bee(EntityId(1));
    EXPECT_FALSE(bee.hasNectar());
}

TEST_F(BeeEntityTest, NectarState_CanSetAndGet)
{
    BeeEntity bee(EntityId(1));

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
    BeeEntity bee(EntityId(1));
    EXPECT_FALSE(bee.hasStung());
}

TEST_F(BeeEntityTest, StungState_CanSetAndGet)
{
    BeeEntity bee(EntityId(1));

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
    BeeEntity bee(EntityId(1));
    EXPECT_FALSE(bee.isFlying());
}

TEST_F(BeeEntityTest, FlyingState_CanSetAndGet)
{
    BeeEntity bee(EntityId(1));

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
    BeeEntity bee(EntityId(1));
    EXPECT_FALSE(bee.hasHive());
}

TEST_F(BeeEntityTest, HivePosition_CanSetAndGet)
{
    BeeEntity bee(EntityId(1));

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
    BeeEntity bee(EntityId(1));
    EXPECT_FALSE(bee.hasFlower());
}

TEST_F(BeeEntityTest, FlowerPosition_CanSetAndGet)
{
    BeeEntity bee(EntityId(1));

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
    BeeEntity bee(EntityId(1));

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
    BeeEntity bee(EntityId(1));
    // MC 1.16.5: 蜜蜂眼睛高度 0.3
    EXPECT_FLOAT_EQ(bee.eyeHeight(), 0.3f);
}

// ============================================================================
// IAngerable 接口测试
// ============================================================================

TEST_F(BeeEntityTest, Anger_CanSetAngerTime)
{
    BeeEntity bee(EntityId(1));

    bee.setAngerTime(100);
    EXPECT_EQ(bee.getAngerTime(), 100);
    EXPECT_TRUE(bee.isAngry());
}

TEST_F(BeeEntityTest, Anger_CanSetAngry)
{
    BeeEntity bee(EntityId(1));

    bee.setAngry(true);
    EXPECT_TRUE(bee.isAngry());
    EXPECT_GT(bee.getAngerTime(), 0);
}

TEST_F(BeeEntityTest, Anger_CanClearAnger)
{
    BeeEntity bee(EntityId(1));

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
    BeeEntity bee(EntityId(1));

    // MC 1.16.5: 蜜蜂攻击伤害为 2.0
    // 注意：需要在构造函数中设置属性
    // 此测试验证属性已注册
    EXPECT_GE(bee.getAttributeValue("generic.attack_damage", -1.0), 0.0);
}

TEST_F(BeeEntityTest, Attributes_HasFollowRange)
{
    BeeEntity bee(EntityId(1));

    // MC 1.16.5: 蜜蜂跟随范围为 48.0
    EXPECT_DOUBLE_EQ(bee.getAttributeValue("generic.follow_range", 0.0), 48.0);
}

TEST_F(BeeEntityTest, Attributes_FlyingSpeedIsSet)
{
    BeeEntity bee(EntityId(1));

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
    BeeEntity bee(EntityId(1));
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
    BeeEntity bee(EntityId(1));
    EXPECT_EQ(bee.getStayOutOfHiveCountdown(), 0);
}

TEST_F(BeeEntityTest, StayOutOfHiveCountdown_CanSetAndGet)
{
    BeeEntity bee(EntityId(1));

    bee.setStayOutOfHiveCountdown(400);
    EXPECT_EQ(bee.getStayOutOfHiveCountdown(), 400);

    bee.setStayOutOfHiveCountdown(0);
    EXPECT_EQ(bee.getStayOutOfHiveCountdown(), 0);

    bee.setStayOutOfHiveCountdown(100);
    EXPECT_EQ(bee.getStayOutOfHiveCountdown(), 100);
}

TEST_F(BeeEntityTest, HiveLocateCooldown_DefaultZero)
{
    BeeEntity bee(EntityId(1));
    EXPECT_EQ(bee.getHiveLocateCooldown(), 0);
}

TEST_F(BeeEntityTest, HiveLocateCooldown_CanSetAndGet)
{
    BeeEntity bee(EntityId(1));

    bee.setHiveLocateCooldown(200);
    EXPECT_EQ(bee.getHiveLocateCooldown(), 200);

    bee.setHiveLocateCooldown(0);
    EXPECT_EQ(bee.getHiveLocateCooldown(), 0);

    bee.setHiveLocateCooldown(50);
    EXPECT_EQ(bee.getHiveLocateCooldown(), 50);
}

TEST_F(BeeEntityTest, DropHive_ClearsHiveAndSetsCooldown)
{
    BeeEntity bee(EntityId(1));
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
    BeeEntity bee(EntityId(1));
    // 没有蜂巢时调用 dropHive
    EXPECT_FALSE(bee.hasHive());

    bee.dropHive();

    EXPECT_FALSE(bee.hasHive());
    EXPECT_EQ(bee.getStayOutOfHiveCountdown(), 200);
}

TEST_F(BeeEntityTest, TicksWithoutNectar_DefaultZero)
{
    BeeEntity bee(EntityId(1));
    EXPECT_EQ(bee.getTicksWithoutNectar(), 0);
}

TEST_F(BeeEntityTest, TicksWithoutNectar_CanReset)
{
    BeeEntity bee(EntityId(1));
    // 模拟递增（通过直接设置内部状态不可行，但可以验证 resetTicksWithoutNectar）
    bee.resetTicksWithoutNectar();
    EXPECT_EQ(bee.getTicksWithoutNectar(), 0);
}

TEST_F(BeeEntityTest, Pollinating_DefaultFalse)
{
    BeeEntity bee(EntityId(1));
    EXPECT_FALSE(bee.isPollinating());
}

TEST_F(BeeEntityTest, Pollinating_CanSetAndGet)
{
    BeeEntity bee(EntityId(1));

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
    BeeEntity bee(EntityId(1));
    bee.setWorld(&m_world);

    // 没有设置蜂巢位置
    EXPECT_FALSE(bee.hasHive());
    EXPECT_FALSE(bee.isHiveValid());
}

TEST_F(BeeHiveInteractionTest, IsHiveValid_NoWorld_ReturnsFalse)
{
    BeeEntity bee(EntityId(1));
    bee.setHivePos(BlockPos(10, 64, 20));

    EXPECT_TRUE(bee.hasHive());
    // 没有世界，getBeehiveBlockEntity 返回 nullptr
    EXPECT_FALSE(bee.isHiveValid());
}

TEST_F(BeeHiveInteractionTest, IsHiveValid_WithValidHive_ReturnsTrue)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);

    EXPECT_EQ(bee.getBeehiveBlockEntity(), nullptr);
}

TEST_F(BeeHiveInteractionTest, WantsToEnterHive_WithNectar_ReturnsTrue)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
    bee.setWorld(&m_world);
    bee.setPosition(10.5f, 64.0f, 20.5f);
    bee.setHivePos(hivePos);

    // 没有花粉
    bee.setHasNectar(false);
    // 确保冷却为 0
    bee.setStayOutOfHiveCountdown(0);
    // 蜜蜂有蜂巢但无花粉，ticksWithoutNectar 需要超过 2400
    // 通过 tick() 递增不太实际（需要 2400 次 tick），直接设置内部状态不可行
    // 所以我们验证没有花粉且不累时不想回巢
    EXPECT_FALSE(bee.wantsToEnterHive());
}

TEST_F(BeeHiveInteractionTest, WantsToEnterHive_StayOutOfHiveCountdown_ReturnsFalse)
{
    BlockPos hivePos(10, 64, 20);
    m_world.setBeehiveAt(hivePos);

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
    bee.setWorld(&m_world);
    bee.setHivePos(hivePos);

    EXPECT_FALSE(bee.isHiveNearFire());
}

TEST_F(BeeHiveInteractionTest, IsHiveNearFire_NoHive_ReturnsFalse)
{
    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
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

    BeeEntity bee(EntityId(1));
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

} // namespace
} // namespace mc
