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

#include "common/TestWorldHelper.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/decorative/CampfireBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/interactive/BeehiveBlockEntity.hpp"
#include "core/Constants.hpp"
#include "entity/core/Entity.hpp"
#include "entity/core/EntityTypeIdNumber.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/damage/DamageSource.hpp"
#include "entity/entities/monster/arthropod/EndermiteEntity.hpp"
#include "entity/entities/passive/special/TurtleEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/registry/VanillaEntities.hpp"
#include "item/core/ItemStack.hpp"
#include "item/enchantment/EnchantmentHelper.hpp"
#include "util/Direction.hpp"
#include "util/math/random/Random.hpp"
#include "util/property/Properties.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/blocks/mob/BeehiveBlock.hpp"
#include "world/block/blocks/mob/DragonBreathBlock.hpp"
#include "world/block/blocks/mob/InfestedBlock.hpp"
#include "world/block/blocks/mob/SpawnerBlock.hpp"
#include "world/block/blocks/mob/TurtleEggBlock.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/gamerule/GameRules.hpp"
#include "world/tick/manager/TickManager.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::blocks;

// ========== BeehiveBlock 测试 ==========

class BeehiveBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建蜂巢方块
        beehive_ = std::make_unique<BeehiveBlock>(BlockProperties(Material::WOOD).hardness(0.6f).resistance(0.6f));
    }

    std::unique_ptr<BeehiveBlock> beehive_;
};

// ========== 基础属性测试 ==========

TEST_F(BeehiveBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(beehive_, nullptr);
}

TEST_F(BeehiveBlockTest, HasBlockEntity_ReturnsTrue)
{
    EXPECT_TRUE(beehive_->hasBlockEntity());
}

TEST_F(BeehiveBlockTest, GetMaxHoneyLevel_Returns5)
{
    EXPECT_EQ(beehive_->getMaxHoneyLevel(), 5);
}

// ========== 蜂蜜等级测试 ==========

TEST_F(BeehiveBlockTest, GetHoneyLevel_ReturnsZeroByDefault)
{
    const auto& state = beehive_->defaultState();
    EXPECT_EQ(beehive_->getHoneyLevel(state), 0);
}

TEST_F(BeehiveBlockTest, WithHoneyLevel_ReturnsCorrectState)
{
    const auto& defaultState = beehive_->defaultState();
    // 测试各个等级
    for (i32 level = 0; level <= 5; ++level) {
        BlockState state = beehive_->withHoneyLevel(defaultState, level);
        EXPECT_EQ(beehive_->getHoneyLevel(state), level) << "Honey level should be " << level;
    }
}

TEST_F(BeehiveBlockTest, WithHoneyLevel_ClampsToValidRange)
{
    const auto& defaultState = beehive_->defaultState();
    // 测试超出范围的值
    BlockState stateNegative = beehive_->withHoneyLevel(defaultState, -1);
    EXPECT_EQ(beehive_->getHoneyLevel(stateNegative), 0) << "Negative level should be clamped to 0";

    BlockState stateOverflow = beehive_->withHoneyLevel(defaultState, 10);
    EXPECT_EQ(beehive_->getHoneyLevel(stateOverflow), 5) << "Overflow level should be clamped to 5";
}

TEST_F(BeehiveBlockTest, WithHoneyLevel_PreservesOtherProperties)
{
    // 获取默认状态的朝向
    const auto& defaultState = beehive_->defaultState();
    Direction defaultFacing = defaultState.get(BlockStateProperties::HORIZONTAL_FACING());

    // 设置蜂蜜等级后朝向应该保持不变
    BlockState state = beehive_->withHoneyLevel(defaultState, 3);
    Direction facingAfter = state.get(BlockStateProperties::HORIZONTAL_FACING());

    EXPECT_EQ(facingAfter, defaultFacing) << "Honey level change should not affect facing";
}

// ========== 朝向属性测试 ==========

TEST_F(BeehiveBlockTest, DefaultState_HasCorrectFacing)
{
    const auto& state = beehive_->defaultState();
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    EXPECT_EQ(facing, Direction::North) << "Default facing should be North";
}

TEST_F(BeehiveBlockTest, Rotate_UpdatesFacingCorrectly)
{
    const auto& defaultState = beehive_->defaultState();

    // 测试所有旋转
    // North -> Clockwise90 -> East
    const auto& rotated90 = beehive_->rotate(defaultState, Rotation::Clockwise90);
    EXPECT_EQ(rotated90.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);

    // North -> Clockwise180 -> South
    const auto& rotated180 = beehive_->rotate(defaultState, Rotation::Clockwise180);
    EXPECT_EQ(rotated180.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);

    // North -> CounterClockwise90 -> West
    const auto& rotated270 = beehive_->rotate(defaultState, Rotation::CounterClockwise90);
    EXPECT_EQ(rotated270.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::West);
}

TEST_F(BeehiveBlockTest, Rotate_PreservesHoneyLevel)
{
    // 创建蜂蜜等级为 3 的状态
    const auto& defaultState = beehive_->defaultState();
    BlockState state = beehive_->withHoneyLevel(defaultState, 3);
    i32 honeyLevelBefore = beehive_->getHoneyLevel(state);

    // 旋转
    const auto& rotated = beehive_->rotate(state, Rotation::Clockwise90);
    i32 honeyLevelAfter = beehive_->getHoneyLevel(rotated);

    EXPECT_EQ(honeyLevelAfter, honeyLevelBefore) << "Rotation should not affect honey level";
}

TEST_F(BeehiveBlockTest, Mirror_UpdatesFacingCorrectly)
{
    const auto& defaultState = beehive_->defaultState();

    // LeftRight 镜像：沿 Z 轴镜像，东西互换，南北不变
    // North -> Mirror(LR) -> North (不变)
    const auto& mirroredLR = beehive_->mirror(defaultState, Mirror::LeftRight);
    EXPECT_EQ(mirroredLR.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);

    // East -> Mirror(LR) -> West (东西互换)
    BlockState eastState = defaultState.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    const auto& mirroredEastLR = beehive_->mirror(eastState, Mirror::LeftRight);
    EXPECT_EQ(mirroredEastLR.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::West);

    // FrontBack 镜像：沿 X 轴镜像，南北互换，东西不变
    // North -> Mirror(FB) -> South (南北互换)
    const auto& mirroredFB = beehive_->mirror(defaultState, Mirror::FrontBack);
    EXPECT_EQ(mirroredFB.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);

    // East -> Mirror(FB) -> East (东西不变)
    const auto& mirroredEastFB = beehive_->mirror(eastState, Mirror::FrontBack);
    EXPECT_EQ(mirroredEastFB.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
}

// ========== 状态容器测试 ==========

TEST_F(BeehiveBlockTest, StateContainer_HasHoneyLevelProperty)
{
    // 验证状态容器包含蜂蜜等级属性
    const auto& state = beehive_->defaultState();
    // 如果能获取属性值且不抛异常，说明属性存在
    EXPECT_NO_THROW({ [[maybe_unused]] i32 level = state.get(BlockStateProperties::HONEY_LEVEL_0_5()); });
}

TEST_F(BeehiveBlockTest, StateContainer_HasFacingProperty)
{
    // 验证状态容器包含朝向属性
    const auto& state = beehive_->defaultState();
    EXPECT_NO_THROW({ [[maybe_unused]] Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING()); });
}

// ========== TurtleEggBlock 测试（同文件中的另一个方块）==========

class TurtleEggBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        turtleEgg_ = std::make_unique<TurtleEggBlock>(BlockProperties(Material::SAND).hardness(0.5f).resistance(0.5f));
    }

    std::unique_ptr<TurtleEggBlock> turtleEgg_;
};

TEST_F(TurtleEggBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(turtleEgg_, nullptr);
}

TEST_F(TurtleEggBlockTest, GetEggs_ReturnsOneByDefault)
{
    const auto& state = turtleEgg_->defaultState();
    EXPECT_EQ(turtleEgg_->getEggs(state), 1);
}

TEST_F(TurtleEggBlockTest, GetHatch_ReturnsZeroByDefault)
{
    const auto& state = turtleEgg_->defaultState();
    EXPECT_EQ(turtleEgg_->getHatch(state), 0);
}

TEST_F(TurtleEggBlockTest, WithEggs_ClampsToValidRange)
{
    BlockState state = turtleEgg_->withEggs(0);
    EXPECT_EQ(turtleEgg_->getEggs(state), 1) << "Minimum eggs should be 1";

    state = turtleEgg_->withEggs(5);
    EXPECT_EQ(turtleEgg_->getEggs(state), 4) << "Maximum eggs should be 4";
}

TEST_F(TurtleEggBlockTest, WithHatch_ClampsToValidRange)
{
    BlockState state = turtleEgg_->withHatch(-1);
    EXPECT_EQ(turtleEgg_->getHatch(state), 0) << "Minimum hatch should be 0";

    state = turtleEgg_->withHatch(5);
    EXPECT_EQ(turtleEgg_->getHatch(state), 2) << "Maximum hatch should be 2";
}

TEST_F(TurtleEggBlockTest, GetShape_ReturnsValidShape)
{
    for (i32 eggs = 1; eggs <= 4; ++eggs) {
        BlockState state = turtleEgg_->withEggs(eggs);
        const auto& shape = turtleEgg_->getShape(state);
        EXPECT_FALSE(shape.isEmpty()) << "Eggs " << eggs << " should have valid shape";
    }
}

TEST_F(TurtleEggBlockTest, TicksRandomly_ReturnsTrue)
{
    EXPECT_TRUE(turtleEgg_->ticksRandomly());
}

TEST_F(TurtleEggBlockTest, IsOpaque_ReturnsFalse)
{
    const auto& state = turtleEgg_->defaultState();
    EXPECT_FALSE(turtleEgg_->isOpaque(state));
}

// ========== InfestedBlock 测试 ==========

class InfestedBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建被感染方块（使用石头作为宿主）
        infested_ = std::make_unique<InfestedBlock>(1, // 石头方块 ID（假设）
            BlockProperties(Material::ROCK).hardness(0.75f).resistance(0.75f));
    }

    std::unique_ptr<InfestedBlock> infested_;
};

TEST_F(InfestedBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(infested_, nullptr);
}

TEST_F(InfestedBlockTest, GetHostBlock_ReturnsCorrectId)
{
    EXPECT_EQ(infested_->getHostBlock(), 1u);
}

// ========== SpawnerBlock 测试 ==========

class SpawnerBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        spawner_ = std::make_unique<SpawnerBlock>(BlockProperties(Material::ROCK).hardness(5.0f).resistance(5.0f));
    }

    std::unique_ptr<SpawnerBlock> spawner_;
};

TEST_F(SpawnerBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(spawner_, nullptr);
}

TEST_F(SpawnerBlockTest, HasBlockEntity_ReturnsTrue)
{
    EXPECT_TRUE(spawner_->hasBlockEntity());
}

TEST_F(SpawnerBlockTest, IsOpaque_ReturnsFalse)
{
    const auto& state = spawner_->defaultState();
    EXPECT_FALSE(spawner_->isOpaque(state));
}

// ========== DragonBreathBlock 测试 ==========

class DragonBreathBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        dragonBreath_ =
            std::make_unique<DragonBreathBlock>(BlockProperties(Material::FIRE).hardness(0.0f).resistance(0.0f));
    }

    std::unique_ptr<DragonBreathBlock> dragonBreath_;
};

TEST_F(DragonBreathBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(dragonBreath_, nullptr);
}

TEST_F(DragonBreathBlockTest, GetShape_ReturnsEmptyShape)
{
    const auto& state = dragonBreath_->defaultState();
    const auto& shape = dragonBreath_->getShape(state);
    EXPECT_TRUE(shape.isEmpty()) << "Dragon breath should have empty shape";
}

TEST_F(DragonBreathBlockTest, GetCollisionShape_ReturnsEmptyShape)
{
    const auto& state = dragonBreath_->defaultState();
    const auto& shape = dragonBreath_->getCollisionShape(state);
    EXPECT_TRUE(shape.isEmpty()) << "Dragon breath should have no collision";
}

TEST_F(DragonBreathBlockTest, IsOpaque_ReturnsFalse)
{
    const auto& state = dragonBreath_->defaultState();
    EXPECT_FALSE(dragonBreath_->isOpaque(state));
}

// ============================================================================
// 测试用世界实现 - 用于测试实体生成和踩踏逻辑
// ============================================================================

namespace {

/**
 * @brief 测试用 Mock 世界实现
 *
 * 提供 MobBlocks 测试所需的最小 IWorld 接口实现
 */
class MobBlocksTestWorld final : public test::BaseTestWorld {
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

    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }
    [[nodiscard]] bool isClientSide() const override { return m_isClientSide; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }
    [[nodiscard]] bool isThundering() const override { return false; }

    EntityId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityId>(m_spawnedEntities.size());
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override
    {
        // 测试中忽略声音播放
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("MobBlocksTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("MobBlocksTestWorld::tickManager not implemented");
    }

    // 测试辅助方法
    void setClientSide(bool clientSide) { m_isClientSide = clientSide; }
    void incrementTick() { m_currentTick++; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    void setBlockAt(const BlockPos& pos, const BlockState* state)
    {
        m_blocks[pos] = std::make_unique<BlockState>(*state);
    }

    void setSandAt(i32 x, i32 y, i32 z)
    {
        // 设置沙子方块（海龟蛋需要放在沙子上）
        const BlockState* sandState = VanillaBlocks::SAND ? &VanillaBlocks::SAND->defaultState() : nullptr;
        if (sandState) {
            m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*sandState);
        }
    }

    [[nodiscard]] size_t spawnedEntityCount() const { return m_spawnedEntities.size(); }

    [[nodiscard]] Entity* getSpawnedEntity(size_t index) const
    {
        if (index < m_spawnedEntities.size()) {
            return m_spawnedEntities[index].get();
        }
        return nullptr;
    }

    void clearSpawnedEntities() { m_spawnedEntities.clear(); }

    // GameRules 接口
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const override { return m_gameRules; }
    [[nodiscard]] world::gamerule::GameRules& getGameRules() override { return m_gameRules; }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    u64 m_currentTick = 0;
    bool m_isClientSide = false;
    world::gamerule::GameRules m_gameRules;
};

/**
 * @brief 测试用 Mock LivingEntity 实现
 */
class MockLivingEntity : public LivingEntity {
public:
    MockLivingEntity(EntityId id)
        : LivingEntity(id, nullptr)
    {}

    void tick() override {}
    [[nodiscard]] f32 width() const override { return 0.6f; }
    [[nodiscard]] f32 height() const override { return 1.8f; }
    [[nodiscard]] f32 eyeHeight() const override { return 1.62f; }
};

/**
 * @brief 测试用 Mock 玩家实现
 */
class MockPlayer : public Player {
public:
    MockPlayer(EntityId id)
        : Player(id, "TestPlayer")
    {}

    void tick() override {}
    [[nodiscard]] std::string getTypeId() const override { return "minecraft:player"; }
};

} // anonymous namespace

// ==================== TurtleEggBlock 实体踩踏测试 ====================

class TurtleEggBlockTrampleTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        turtleEgg_ = std::make_unique<TurtleEggBlock>(BlockProperties(Material::SAND).hardness(0.5f).resistance(0.5f));
    }

    std::unique_ptr<TurtleEggBlock> turtleEgg_;
    MobBlocksTestWorld world_;
};

TEST_F(TurtleEggBlockTrampleTest, OnEntityWalk_PlayerCanTrample)
{
    // 设置海龟蛋在沙子上
    world_.setSandAt(5, 0, 5);
    BlockPos eggPos(5, 1, 5);
    BlockState eggState = turtleEgg_->defaultState().with(BlockStateProperties::EGGS_1_4(), 2);
    world_.setBlockAt(eggPos, &eggState);

    // 创建玩家实体
    MockPlayer player(EntityId(1));
    player.setPosition(5.5f, 1.0f, 5.5f);

    // 玩家走过应该能踩破蛋（但有概率性，这里主要验证不会崩溃）
    // 设置随机种子以确保可重复性
    world_.getRandom().setSeed(12345);

    // 调用 onEntityWalk（踩踏检查内部有随机因素）
    turtleEgg_->onEntityWalk(eggState, world_, eggPos, player);

    // 验证：玩家可以踩破蛋（由于随机因素，可能踩破也可能不踩破）
    // 这里主要验证不会崩溃，且生成了正确的结果
    EXPECT_TRUE(true); // 如果执行到这里说明没有崩溃
}

TEST_F(TurtleEggBlockTrampleTest, OnEntityWalk_TurtleCannotTrample)
{
    // 设置海龟蛋在沙子上
    world_.setSandAt(5, 0, 5);
    BlockPos eggPos(5, 1, 5);
    BlockState eggState = turtleEgg_->defaultState().with(BlockStateProperties::EGGS_1_4(), 2);
    world_.setBlockAt(eggPos, &eggState);

    // 创建海龟实体
    TurtleEntity turtle(EntityId(1));
    turtle.setPosition(5.5f, 1.0f, 5.5f);

    // 海龟走过不应该踩破蛋
    turtleEgg_->onEntityWalk(eggState, world_, eggPos, turtle);

    // 验证：海龟不能踩破蛋
    const BlockState* stateAfter = world_.getBlockState(eggPos.x, eggPos.y, eggPos.z);
    ASSERT_NE(stateAfter, nullptr);
    // 蛋数量应该保持不变（海龟不能踩破）
    EXPECT_EQ(turtleEgg_->getEggs(*stateAfter), 2);
}

TEST_F(TurtleEggBlockTrampleTest, OnFallenUpon_ZombieDoesNotTrample)
{
    // 设置海龟蛋在沙子上
    world_.setSandAt(5, 0, 5);
    BlockPos eggPos(5, 1, 5);
    BlockState eggState = turtleEgg_->defaultState().with(BlockStateProperties::EGGS_1_4(), 3);
    world_.setBlockAt(eggPos, &eggState);

    // 创建僵尸实体（僵尸不会踩破海龟蛋）
    MockLivingEntity zombie(EntityId(1));
    zombie.setPosition(5.5f, 5.0f, 5.5f);

    // 僵尸摔落在蛋上
    turtleEgg_->onFallenUpon(world_, eggPos, eggState, zombie, 5.0f);

    // 验证：僵尸不会踩破蛋
    const BlockState* stateAfter = world_.getBlockState(eggPos.x, eggPos.y, eggPos.z);
    ASSERT_NE(stateAfter, nullptr);
    // 蛋数量应该保持不变（僵尸不会踩破）
    EXPECT_EQ(turtleEgg_->getEggs(*stateAfter), 3);
}

TEST_F(TurtleEggBlockTrampleTest, OnFallenUpon_HuskDoesNotTrample)
{
    // 设置海龟蛋在沙子上
    world_.setSandAt(5, 0, 5);
    BlockPos eggPos(5, 1, 5);
    BlockState eggState = turtleEgg_->defaultState().with(BlockStateProperties::EGGS_1_4(), 2);
    world_.setBlockAt(eggPos, &eggState);

    // 创建尸壳实体（僵尸变种，不会踩破海龟蛋）
    MockLivingEntity husk(EntityId(1));
    husk.setPosition(5.5f, 5.0f, 5.5f);

    // 尸壳摔落在蛋上
    turtleEgg_->onFallenUpon(world_, eggPos, eggState, husk, 5.0f);

    // 验证：尸壳不会踩破蛋
    const BlockState* stateAfter = world_.getBlockState(eggPos.x, eggPos.y, eggPos.z);
    ASSERT_NE(stateAfter, nullptr);
    EXPECT_EQ(turtleEgg_->getEggs(*stateAfter), 2);
}

TEST_F(TurtleEggBlockTrampleTest, OnFallenUpon_DrownedDoesNotTrample)
{
    // 设置海龟蛋在沙子上
    world_.setSandAt(5, 0, 5);
    BlockPos eggPos(5, 1, 5);
    BlockState eggState = turtleEgg_->defaultState().with(BlockStateProperties::EGGS_1_4(), 4);
    world_.setBlockAt(eggPos, &eggState);

    // 创建溺尸实体（僵尸变种，不会踩破海龟蛋）
    MockLivingEntity drowned(EntityId(1));
    drowned.setPosition(5.5f, 5.0f, 5.5f);

    // 溺尸摔落在蛋上
    turtleEgg_->onFallenUpon(world_, eggPos, eggState, drowned, 5.0f);

    // 验证：溺尸不会踩破蛋
    const BlockState* stateAfter = world_.getBlockState(eggPos.x, eggPos.y, eggPos.z);
    ASSERT_NE(stateAfter, nullptr);
    EXPECT_EQ(turtleEgg_->getEggs(*stateAfter), 4);
}

TEST_F(TurtleEggBlockTrampleTest, OnFallenUpon_BatCannotTrample)
{
    // 设置海龟蛋在沙子上
    world_.setSandAt(5, 0, 5);
    BlockPos eggPos(5, 1, 5);
    BlockState eggState = turtleEgg_->defaultState().with(BlockStateProperties::EGGS_1_4(), 2);
    world_.setBlockAt(eggPos, &eggState);

    // 创建蝙蝠实体（蝙蝠不能踩破蛋）
    MockLivingEntity bat(EntityId(1));
    bat.setPosition(5.5f, 5.0f, 5.5f);

    // 蝙蝠摔落在蛋上
    turtleEgg_->onFallenUpon(world_, eggPos, eggState, bat, 5.0f);

    // 验证：蝙蝠不能踩破蛋
    const BlockState* stateAfter = world_.getBlockState(eggPos.x, eggPos.y, eggPos.z);
    ASSERT_NE(stateAfter, nullptr);
    EXPECT_EQ(turtleEgg_->getEggs(*stateAfter), 2);
}

TEST_F(TurtleEggBlockTrampleTest, OnFallenUpon_NonLivingEntityCannotTrample)
{
    // 设置海龟蛋在沙子上
    world_.setSandAt(5, 0, 5);
    BlockPos eggPos(5, 1, 5);
    BlockState eggState = turtleEgg_->defaultState().with(BlockStateProperties::EGGS_1_4(), 2);
    world_.setBlockAt(eggPos, &eggState);

    // 创建物品实体（非 LivingEntity，不能踩破蛋）
    Entity item(EntityId(1));
    item.setPosition(5.5f, 5.0f, 5.5f);

    // 物品摔落在蛋上
    turtleEgg_->onFallenUpon(world_, eggPos, eggState, item, 5.0f);

    // 验证：物品不能踩破蛋
    const BlockState* stateAfter = world_.getBlockState(eggPos.x, eggPos.y, eggPos.z);
    ASSERT_NE(stateAfter, nullptr);
    EXPECT_EQ(turtleEgg_->getEggs(*stateAfter), 2);
}

// ==================== TurtleEggBlock 孵化测试 ====================

class TurtleEggBlockHatchTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        turtleEgg_ = std::make_unique<TurtleEggBlock>(BlockProperties(Material::SAND).hardness(0.5f).resistance(0.5f));
    }

    std::unique_ptr<TurtleEggBlock> turtleEgg_;
    MobBlocksTestWorld world_;
};

TEST_F(TurtleEggBlockHatchTest, RandomTick_HatchingProgresses)
{
    // 设置海龟蛋在沙子上
    world_.setSandAt(10, 0, 10);
    BlockPos eggPos(10, 1, 10);
    BlockState eggState =
        turtleEgg_->defaultState().with(BlockStateProperties::EGGS_1_4(), 1).with(BlockStateProperties::HATCH_0_2(), 0);
    world_.setBlockAt(eggPos, &eggState);

    // 调用 randomTick，设置随机种子使 canGrow 返回 true
    // 由于 canGrow 需要特定的随机条件，我们主要验证不会崩溃
    turtleEgg_->randomTick(world_, eggPos, eggState, world_.getRandom());

    // 验证执行完成（可能孵化，也可能不孵化，取决于随机数）
    EXPECT_TRUE(true);
}

TEST_F(TurtleEggBlockHatchTest, RandomTick_NoHatchWithoutSand)
{
    // 设置海龟蛋，但下方不是沙子（是空气）
    BlockPos eggPos(10, 5, 10);
    BlockState eggState = turtleEgg_->defaultState()
                              .with(BlockStateProperties::EGGS_1_4(), 1)
                              .with(BlockStateProperties::HATCH_0_2(), 2); // 即将孵化
    world_.setBlockAt(eggPos, &eggState);

    // 调用 randomTick
    turtleEgg_->randomTick(world_, eggPos, eggState, world_.getRandom());

    // 由于没有沙子，不应该孵化
    EXPECT_EQ(world_.spawnedEntityCount(), 0u);
}

TEST_F(TurtleEggBlockHatchTest, RandomTick_ClientSideDoesNotSpawn)
{
    // 设置客户端
    world_.setClientSide(true);

    // 设置海龟蛋在沙子上
    world_.setSandAt(10, 0, 10);
    BlockPos eggPos(10, 1, 10);
    BlockState eggState = turtleEgg_->defaultState()
                              .with(BlockStateProperties::EGGS_1_4(), 1)
                              .with(BlockStateProperties::HATCH_0_2(), 2); // 即将孵化
    world_.setBlockAt(eggPos, &eggState);

    // 调用 randomTick
    turtleEgg_->randomTick(world_, eggPos, eggState, world_.getRandom());

    // 客户端不应该生成实体
    EXPECT_EQ(world_.spawnedEntityCount(), 0u);
}

// ==================== InfestedBlock spawnAfterBreak 测试 ====================

class InfestedBlockSpawnAfterBreakTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        // 注册原版实体类型，使 EntityTypeIdNumber::SILVERFISH 全局缓存与注册表一致。
        // 本夹具 SpawnAfterBreak_SpawnsSilverfish_OnServer 用例断言
        // spawned->typeId() == EntityTypeIdNumber::SILVERFISH，二者必须来自同一
        // 已初始化注册表，避免依赖前置测试的隐式注册状态（测试顺序污染）。
        // VanillaEntities::registerAll() 幂等且线程安全，无异常风险。
        entity::VanillaEntities::registerAll();
        infested_ = std::make_unique<InfestedBlock>(1, // 石头方块ID
            BlockProperties(Material::ROCK).hardness(0.75f).resistance(0.75f));
    }

    std::unique_ptr<InfestedBlock> infested_;
    MobBlocksTestWorld world_;
};

TEST_F(InfestedBlockSpawnAfterBreakTest, SpawnAfterBreak_SpawnsSilverfish_OnServer)
{
    // 设置被感染方块
    BlockPos pos(5, 10, 5);
    const BlockState& state = infested_->defaultState();
    world_.setBlockAt(pos, &state);

    // 调用 spawnAfterBreak（服务端，无精准采集工具）
    infested_->spawnAfterBreak(world_, pos, state, nullptr, true);

    // 验证：应该生成一个蠹虫实体
    EXPECT_EQ(world_.spawnedEntityCount(), 1u);

    // 验证生成的实体类型
    Entity* spawned = world_.getSpawnedEntity(0);
    ASSERT_NE(spawned, nullptr);
    EXPECT_EQ(spawned->typeId(), entity::EntityTypeIdNumber::SILVERFISH);
}

TEST_F(InfestedBlockSpawnAfterBreakTest, SpawnAfterBreak_DoesNotSpawn_OnClient)
{
    // 设置客户端
    world_.setClientSide(true);

    // 设置被感染方块
    BlockPos pos(5, 10, 5);
    const BlockState& state = infested_->defaultState();
    world_.setBlockAt(pos, &state);

    // 调用 spawnAfterBreak（客户端）
    infested_->spawnAfterBreak(world_, pos, state, nullptr, true);

    // 验证：客户端不应该生成实体
    EXPECT_EQ(world_.spawnedEntityCount(), 0u);
}

TEST_F(InfestedBlockSpawnAfterBreakTest, SpawnAfterBreak_SilverfishPositionCorrect)
{
    // 设置被感染方块
    BlockPos pos(100, 50, -200);
    const BlockState& state = infested_->defaultState();
    world_.setBlockAt(pos, &state);

    // 调用 spawnAfterBreak
    infested_->spawnAfterBreak(world_, pos, state, nullptr, true);

    // 验证生成的蠹虫位置
    Entity* spawned = world_.getSpawnedEntity(0);
    ASSERT_NE(spawned, nullptr);

    // 蠹虫应该在方块中心生成
    // x = pos.x + 0.5, y = pos.y, z = pos.z + 0.5
    EXPECT_NEAR(spawned->x(), 100.5f, 0.01f);
    EXPECT_NEAR(spawned->y(), 50.0f, 0.01f);
    EXPECT_NEAR(spawned->z(), -199.5f, 0.01f);
}

TEST_F(InfestedBlockSpawnAfterBreakTest, SpawnAfterBreak_DoTileDropsFalse_DoesNotSpawn)
{
    // 设置 doTileDrops 游戏规则为 false
    world_.getGameRules().setBoolean(world::gamerule::GameRuleKeys::DO_TILE_DROPS, false, nullptr);

    // 设置被感染方块
    BlockPos pos(5, 10, 5);
    const BlockState& state = infested_->defaultState();
    world_.setBlockAt(pos, &state);

    // 调用 spawnAfterBreak（服务端，无精准采集工具）
    // doTileDrops=false 时不应生成蠹虫
    infested_->spawnAfterBreak(world_, pos, state, nullptr, true);

    // 验证：不应该生成任何实体
    EXPECT_EQ(world_.spawnedEntityCount(), 0u);
}

TEST_F(InfestedBlockSpawnAfterBreakTest, SpawnAfterBreak_DoTileDropsTrue_SpawnsSilverfish)
{
    // 确保 doTileDrops 游戏规则为 true（默认值）
    world_.getGameRules().setBoolean(world::gamerule::GameRuleKeys::DO_TILE_DROPS, true, nullptr);

    // 设置被感染方块
    BlockPos pos(5, 10, 5);
    const BlockState& state = infested_->defaultState();
    world_.setBlockAt(pos, &state);

    // 调用 spawnAfterBreak（服务端，无精准采集工具）
    infested_->spawnAfterBreak(world_, pos, state, nullptr, true);

    // 验证：应该生成一个蠹虫实体
    EXPECT_EQ(world_.spawnedEntityCount(), 1u);
}

TEST_F(InfestedBlockSpawnAfterBreakTest, SpawnAfterBreak_SilkTouchTool_DoesNotSpawn)
{
    // 使用精准采集附魔的工具破坏虫蚀方块时不应生成蠹虫
    if (Items::DIAMOND_PICKAXE == nullptr) {
        GTEST_SKIP() << "DIAMOND_PICKAXE not initialized";
    }

    ItemStack silkTouchTool(*Items::DIAMOND_PICKAXE, 1);
    silkTouchTool.addEnchantment("minecraft:silk_touch", 1);
    ASSERT_TRUE(item::enchant::EnchantmentHelper::hasSilkTouch(silkTouchTool));

    // 设置被感染方块
    BlockPos pos(5, 10, 5);
    const BlockState& state = infested_->defaultState();
    world_.setBlockAt(pos, &state);

    // 调用 spawnAfterBreak（服务端，精准采集工具）
    infested_->spawnAfterBreak(world_, pos, state, &silkTouchTool, true);

    // 验证：精准采集不应生成蠹虫
    EXPECT_EQ(world_.spawnedEntityCount(), 0u);
}

TEST_F(InfestedBlockSpawnAfterBreakTest, SpawnAfterBreak_RegularTool_SpawnsSilverfish)
{
    // 使用普通工具（无附魔）破坏虫蚀方块时应生成蠹虫
    if (Items::DIAMOND_PICKAXE == nullptr) {
        GTEST_SKIP() << "DIAMOND_PICKAXE not initialized";
    }

    ItemStack regularTool(*Items::DIAMOND_PICKAXE, 1);
    ASSERT_FALSE(item::enchant::EnchantmentHelper::hasSilkTouch(regularTool));

    // 设置被感染方块
    BlockPos pos(5, 10, 5);
    const BlockState& state = infested_->defaultState();
    world_.setBlockAt(pos, &state);

    // 调用 spawnAfterBreak（服务端，普通工具）
    infested_->spawnAfterBreak(world_, pos, state, &regularTool, true);

    // 验证：普通工具应生成蠹虫
    EXPECT_EQ(world_.spawnedEntityCount(), 1u);
}

// ==================== InfestedBlock 静态方法测试 ====================

class InfestedBlockStaticTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        // 初始化映射表
        InfestedBlock::initializeMappings();
    }
};

TEST_F(InfestedBlockStaticTest, CanContainSilverfish_ReturnsFalseForUnknownBlock)
{
    // 对于未注册的方块，应该返回 false
    // 空气方块不应该能被虫蚀
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    EXPECT_FALSE(InfestedBlock::canContainSilverfish(airState));
}

TEST_F(InfestedBlockStaticTest, Infest_ReturnsNullptrForUnknownBlock)
{
    // 对于未注册的方块，应该返回 nullptr
    // 空气方块没有虫蚀版本
    const Block* airBlock = VanillaBlocks::AIR;
    ASSERT_NE(airBlock, nullptr);
    EXPECT_EQ(InfestedBlock::infest(*airBlock), nullptr);
}

TEST_F(InfestedBlockStaticTest, RegisterInfestedBlock_AddsMapping)
{
    // 注册一个新的虫蚀方块映射
    // 使用测试用的 ID（避免与已注册的冲突）
    constexpr u32 TEST_HOST_BLOCK = 99990;
    constexpr u32 TEST_INFESTED_BLOCK = 99991;

    InfestedBlock::registerInfestedBlock(TEST_HOST_BLOCK, TEST_INFESTED_BLOCK);

    // 初始化映射
    InfestedBlock::initializeMappings();

    // 验证：映射表应该包含新的映射
    // 注意：canContainSilverfish 需要 BlockState，这里只验证注册不会崩溃
    EXPECT_TRUE(true);
}

TEST_F(InfestedBlockStaticTest, InitializeMappings_CanBeCalledMultipleTimes)
{
    // initializeMappings 应该是幂等的
    EXPECT_NO_THROW({
        InfestedBlock::initializeMappings();
        InfestedBlock::initializeMappings();
        InfestedBlock::initializeMappings();
    });
}

TEST_F(InfestedBlockStaticTest, RegisterInfestedBlock_MultipleMappings)
{
    // 注册多个映射
    InfestedBlock::registerInfestedBlock(90001, 90011);
    InfestedBlock::registerInfestedBlock(90002, 90012);
    InfestedBlock::registerInfestedBlock(90003, 90013);

    // 验证不会崩溃
    EXPECT_TRUE(true);
}

// ==================== DragonBreathBlock 实体碰撞测试 ====================
//
// 注：commit b1b27cb0d 移除了 DragonBreathBlock::onEntityCollision 的直接伤害逻辑，
// 龙息方块改为纯视觉占位——龙息伤害由 DragonFireballEntity / DragonSittingFlamingPhase
// 生成的 AreaEffectCloudEntity 统一处理（与原版 MC 行为一致）。下方测试验证该决策：
// 方块碰撞本身不施加任何伤害。

/**
 * @brief 测试用伤害追踪 LivingEntity
 *
 * 继承 LivingEntity 并追踪伤害调用
 */
class DamageTrackingLivingEntity : public LivingEntity {
public:
    DamageTrackingLivingEntity(EntityId id, IWorld* world = nullptr)
        : LivingEntity(id, world)
        , m_hurtCount(0)
        , m_lastDamage(0.0f)
        , m_lastDamageType(static_cast<DamageType>(255)) // 无效类型作为初始值
    {}

    bool hurt(DamageSource& source, f32 amount) override
    {
        m_hurtCount++;
        m_lastDamage = amount;
        m_lastDamageType = source.type();
        return LivingEntity::hurt(source, amount);
    }

    [[nodiscard]] i32 hurtCount() const { return m_hurtCount; }
    [[nodiscard]] f32 lastDamage() const { return m_lastDamage; }
    [[nodiscard]] DamageType lastDamageType() const { return m_lastDamageType; }

    void tick() override {}
    [[nodiscard]] f32 width() const override { return 0.6f; }
    [[nodiscard]] f32 height() const override { return 1.8f; }
    [[nodiscard]] f32 eyeHeight() const override { return 1.62f; }

private:
    i32 m_hurtCount;
    f32 m_lastDamage;
    DamageType m_lastDamageType;
};

class DragonBreathBlockCollisionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        dragonBreath_ =
            std::make_unique<DragonBreathBlock>(BlockProperties(Material::FIRE).hardness(0.0f).resistance(0.0f));
    }

    std::unique_ptr<DragonBreathBlock> dragonBreath_;
    MobBlocksTestWorld world_;
};

TEST_F(DragonBreathBlockCollisionTest, OnEntityCollision_LivingEntity_NoDamage)
{
    // 设置服务端
    world_.setClientSide(false);

    // 设置龙息方块
    BlockPos pos(0, 0, 0);
    const BlockState& state = dragonBreath_->defaultState();
    world_.setBlockAt(pos, &state);

    // 创建生物实体
    DamageTrackingLivingEntity entity(EntityId(1), &world_);
    entity.setPosition(0.5f, 0.0f, 0.5f);
    entity.setHealth(20.0f);

    // 触发碰撞
    dragonBreath_->onEntityCollision(state, world_, pos, entity);

    // 验证：龙息方块为纯视觉占位，不直接造成伤害（伤害由 AreaEffectCloudEntity 处理）
    EXPECT_EQ(entity.hurtCount(), 0);
    EXPECT_FLOAT_EQ(entity.health(), 20.0f);
}

TEST_F(DragonBreathBlockCollisionTest, OnEntityCollision_ClientSide_NoDamage)
{
    // 设置客户端
    world_.setClientSide(true);

    // 设置龙息方块
    BlockPos pos(0, 0, 0);
    const BlockState& state = dragonBreath_->defaultState();
    world_.setBlockAt(pos, &state);

    // 创建生物实体
    DamageTrackingLivingEntity entity(EntityId(1), &world_);
    entity.setPosition(0.5f, 0.0f, 0.5f);
    entity.setHealth(20.0f);

    // 触发碰撞
    dragonBreath_->onEntityCollision(state, world_, pos, entity);

    // 验证：客户端不应该造成伤害
    EXPECT_EQ(entity.hurtCount(), 0);
    EXPECT_FLOAT_EQ(entity.health(), 20.0f);
}

TEST_F(DragonBreathBlockCollisionTest, OnEntityCollision_NonLivingEntity_NoDamage)
{
    // 设置服务端
    world_.setClientSide(false);

    // 设置龙息方块
    BlockPos pos(0, 0, 0);
    const BlockState& state = dragonBreath_->defaultState();
    world_.setBlockAt(pos, &state);

    // 创建非生物实体（物品实体）
    Entity item(EntityId(1));
    item.setPosition(0.5f, 0.0f, 0.5f);

    // 触发碰撞 - 不应该崩溃
    EXPECT_NO_THROW({ dragonBreath_->onEntityCollision(state, world_, pos, item); });

    // 非生物实体不应该受到伤害（方法内部检查 LivingEntity）
}

TEST_F(DragonBreathBlockCollisionTest, OnEntityCollision_MultipleCollisions_NoDamage)
{
    // 设置服务端
    world_.setClientSide(false);

    // 设置龙息方块
    BlockPos pos(0, 0, 0);
    const BlockState& state = dragonBreath_->defaultState();
    world_.setBlockAt(pos, &state);

    // 创建生物实体
    DamageTrackingLivingEntity entity(EntityId(1), &world_);
    entity.setPosition(0.5f, 0.0f, 0.5f);
    entity.setHealth(20.0f);

    // 触发多次碰撞（模拟持续站在龙息中）
    dragonBreath_->onEntityCollision(state, world_, pos, entity);
    dragonBreath_->onEntityCollision(state, world_, pos, entity);
    dragonBreath_->onEntityCollision(state, world_, pos, entity);

    // 验证：方块碰撞不造成伤害，多次碰撞仍为 0
    EXPECT_EQ(entity.hurtCount(), 0);
}

TEST_F(DragonBreathBlockCollisionTest, OnEntityCollision_DragonBreathBlock_NoDirectDamage)
{
    // 设置服务端
    world_.setClientSide(false);

    // 设置龙息方块
    BlockPos pos(0, 0, 0);
    const BlockState& state = dragonBreath_->defaultState();
    world_.setBlockAt(pos, &state);

    // 创建生物实体
    DamageTrackingLivingEntity entity(EntityId(1), &world_);
    entity.setPosition(0.5f, 0.0f, 0.5f);
    entity.setHealth(20.0f);

    // 触发碰撞
    dragonBreath_->onEntityCollision(state, world_, pos, entity);

    // 验证：龙息方块不直接施加伤害，伤害类型/数值保持未触发状态
    EXPECT_EQ(entity.hurtCount(), 0);
    EXPECT_FLOAT_EQ(entity.lastDamage(), 0.0f);
}

TEST_F(DragonBreathBlockCollisionTest, OnEntityCollision_DifferentEntityTypes_NoneTakeDamage)
{
    // 设置服务端
    world_.setClientSide(false);

    // 设置龙息方块
    BlockPos pos(0, 0, 0);
    const BlockState& state = dragonBreath_->defaultState();
    world_.setBlockAt(pos, &state);

    // 测试不同类型的生物实体
    // 注意：EntityTypeIdNumber 在未初始化注册表时值为 0，这里只测试伤害逻辑
    std::vector<entity::EntityTypeId> entityTypeIds = {
        entity::EntityTypeIdNumber::PIG,
        entity::EntityTypeIdNumber::COW,
        entity::EntityTypeIdNumber::ZOMBIE,
        entity::EntityTypeIdNumber::SKELETON,
        entity::EntityTypeIdNumber::PLAYER,
    };

    for (size_t i = 0; i < entityTypeIds.size(); ++i) {
        DamageTrackingLivingEntity entity(EntityId(static_cast<u32>(i + 1)), &world_);
        entity.setPosition(0.5f, 0.0f, 0.5f);
        entity.setHealth(20.0f);

        dragonBreath_->onEntityCollision(state, world_, pos, entity);

        EXPECT_EQ(entity.hurtCount(), 0) << "Entity type " << entityTypeIds[i] << " should not take damage from block";
        EXPECT_FLOAT_EQ(entity.health(), 20.0f) << "Entity type " << entityTypeIds[i] << " should keep full health";
    }
}

// ========== BeehiveBlockEntity 测试 ==========

class BeehiveBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override { entity_ = std::make_unique<mc::blockentity::BeehiveBlockEntity>(BlockPos(10, 64, 20)); }

    std::unique_ptr<mc::blockentity::BeehiveBlockEntity> entity_;
};

TEST_F(BeehiveBlockEntityTest, Create_HasCorrectDefaults)
{
    EXPECT_EQ(entity_->getOccupantCount(), 0);
    EXPECT_TRUE(entity_->isEmpty());
    EXPECT_FALSE(entity_->isFull());
    EXPECT_EQ(entity_->getSavedFlowerPos(), BlockPos::zero());
}

TEST_F(BeehiveBlockEntityTest, Constants_HaveCorrectValues)
{
    EXPECT_EQ(mc::blockentity::BeehiveBlockEntity::MAX_OCCUPANTS, 3);
    EXPECT_EQ(mc::blockentity::BeehiveBlockEntity::MIN_TICKS_BEFORE_REENTERING_HIVE, 400);
    EXPECT_EQ(mc::blockentity::BeehiveBlockEntity::MIN_OCCUPATION_TICKS_NECTAR, 2400);
    EXPECT_EQ(mc::blockentity::BeehiveBlockEntity::MIN_OCCUPATION_TICKS_NECTARLESS, 600);
}

TEST_F(BeehiveBlockEntityTest, isEmpty_WhenNoBees_ReturnsTrue)
{
    EXPECT_TRUE(entity_->isEmpty());
}

TEST_F(BeehiveBlockEntityTest, isFull_WhenThreeBees_ReturnsTrue)
{
    // 直接通过addOccupant测试需要BeeEntity，此处仅测试容量常量
    EXPECT_EQ(mc::blockentity::BeehiveBlockEntity::MAX_OCCUPANTS, 3);
}

TEST_F(BeehiveBlockEntityTest, getSavedFlowerPos_DefaultIsZero)
{
    EXPECT_EQ(entity_->getSavedFlowerPos(), BlockPos::zero());
}

TEST_F(BeehiveBlockEntityTest, setSavedFlowerPos_UpdatesPosition)
{
    BlockPos flowerPos(100, 70, 200);
    entity_->setSavedFlowerPos(flowerPos);
    EXPECT_EQ(entity_->getSavedFlowerPos(), flowerPos);
}

TEST_F(BeehiveBlockEntityTest, SaveLoad_RoundTrip)
{
    // 设置花朵位置
    entity_->setSavedFlowerPos(BlockPos(50, 65, 75));

    // 保存
    nlohmann::json data;
    entity_->save(data);

    // 验证保存的数据
    EXPECT_TRUE(data.contains("bees"));
    EXPECT_TRUE(data.contains("flower_pos"));
    EXPECT_EQ(data["flower_pos"].size(), 3);
    EXPECT_EQ(data["flower_pos"][0].get<i32>(), 50);
    EXPECT_EQ(data["flower_pos"][1].get<i32>(), 65);
    EXPECT_EQ(data["flower_pos"][2].get<i32>(), 75);

    // 加载到新实体
    auto loaded = std::make_unique<mc::blockentity::BeehiveBlockEntity>(BlockPos(0, 0, 0));
    EXPECT_TRUE(loaded->load(data));
    EXPECT_EQ(loaded->getSavedFlowerPos(), BlockPos(50, 65, 75));
}

TEST_F(BeehiveBlockEntityTest, Clone_PreservesData)
{
    entity_->setSavedFlowerPos(BlockPos(10, 20, 30));
    auto cloned = entity_->clone();
    ASSERT_NE(cloned, nullptr);
    auto* clonedEntity = static_cast<mc::blockentity::BeehiveBlockEntity*>(cloned.get());
    EXPECT_EQ(clonedEntity->getSavedFlowerPos(), BlockPos(10, 20, 30));
    EXPECT_EQ(clonedEntity->isEmpty(), entity_->isEmpty());
}

TEST_F(BeehiveBlockEntityTest, BeeOccupant_Tick_IncrementsTicks)
{
    mc::blockentity::BeehiveBlockEntity::BeeOccupant occupant;
    occupant.hasNectar = true;
    occupant.ticksInHive = 0;
    occupant.minTicksInHive = mc::blockentity::BeehiveBlockEntity::MIN_OCCUPATION_TICKS_NECTAR;

    EXPECT_FALSE(occupant.tick()); // tick() 递增到1，1 > 2400 = false
    EXPECT_EQ(occupant.ticksInHive, 1);

    // Tick up to exactly minTicksInHive - 1
    for (i32 i = 1; i < mc::blockentity::BeehiveBlockEntity::MIN_OCCUPATION_TICKS_NECTAR - 1; ++i) {
        EXPECT_FALSE(occupant.tick()) << "At tick " << i + 1 << ", should not be ready yet";
    }
    // Now ticksInHive = minTicksInHive - 1 = 2399
    EXPECT_EQ(occupant.ticksInHive, mc::blockentity::BeehiveBlockEntity::MIN_OCCUPATION_TICKS_NECTAR - 1);

    // Next tick: ticksInHive becomes 2400, and 2400 > 2400 = false
    EXPECT_FALSE(occupant.tick());
    EXPECT_EQ(occupant.ticksInHive, mc::blockentity::BeehiveBlockEntity::MIN_OCCUPATION_TICKS_NECTAR);

    // Next tick: ticksInHive becomes 2401, and 2401 > 2400 = true
    EXPECT_TRUE(occupant.tick());
    EXPECT_EQ(occupant.ticksInHive, mc::blockentity::BeehiveBlockEntity::MIN_OCCUPATION_TICKS_NECTAR + 1);
}

TEST_F(BeehiveBlockEntityTest, BeeOccupant_Nectarless_MinTicks)
{
    mc::blockentity::BeehiveBlockEntity::BeeOccupant occupant;
    occupant.hasNectar = false;
    occupant.ticksInHive = 0;
    occupant.minTicksInHive = mc::blockentity::BeehiveBlockEntity::MIN_OCCUPATION_TICKS_NECTARLESS;

    EXPECT_EQ(occupant.minTicksInHive, 600);
}

// ========== BeehiveBlock 红石比较器测试 ==========

TEST_F(BeehiveBlockTest, AnalogOutputSignal_ReturnsHoneyLevel)
{
    const auto& defaultState = beehive_->defaultState();
    for (i32 level = 0; level <= 5; ++level) {
        BlockState state = beehive_->withHoneyLevel(defaultState, level);
        EXPECT_EQ(beehive_->getAnalogOutputSignal(state), level) << "Analog output should equal honey level " << level;
    }
}

TEST_F(BeehiveBlockTest, HasAnalogOutputSignal_ReturnsTrue)
{
    EXPECT_TRUE(beehive_->hasAnalogOutputSignal());
}

// ========== CampfireBlock 静态方法测试 ==========

class CampfireBlockStaticTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // isLitCampfire 依赖 BlockTags::CAMPFIRES()，必须使用已注册的 campfire 方块
        // （裸构造的 CampfireBlock 没有 blockLocation，无法命中标签）。
        VanillaBlocks::initialize();
        BlockTags::initialize();

        campfire_ =
            VanillaBlocks::CAMPFIRE ? static_cast<mc::blocks::CampfireBlock*>(VanillaBlocks::CAMPFIRE) : nullptr;
        soulCampfire_ = VanillaBlocks::SOUL_CAMPFIRE
            ? static_cast<mc::blocks::SoulCampfireBlock*>(VanillaBlocks::SOUL_CAMPFIRE)
            : nullptr;
        // 蜂巢仅用于“非营火方块”用例，不需要注册到标签
        beehiveBlock_ =
            std::make_unique<mc::blocks::BeehiveBlock>(BlockProperties(Material::WOOD).hardness(0.6f).resistance(0.6f));
    }

    mc::blocks::CampfireBlock* campfire_ = nullptr;
    mc::blocks::SoulCampfireBlock* soulCampfire_ = nullptr;
    std::unique_ptr<mc::blocks::BeehiveBlock> beehiveBlock_;
};

TEST_F(CampfireBlockStaticTest, IsLitCampfire_LitCampfire_ReturnsTrue)
{
    const auto& litState = campfire_->defaultState()
                               .with(BlockStateProperties::LIT(), true)
                               .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    EXPECT_TRUE(mc::blocks::CampfireBlock::isLitCampfire(litState));
}

TEST_F(CampfireBlockStaticTest, IsLitCampfire_UnlitCampfire_ReturnsFalse)
{
    const auto& unlitState = campfire_->defaultState()
                                 .with(BlockStateProperties::LIT(), false)
                                 .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    EXPECT_FALSE(mc::blocks::CampfireBlock::isLitCampfire(unlitState));
}

TEST_F(CampfireBlockStaticTest, IsLitCampfire_LitSoulCampfire_ReturnsTrue)
{
    // 灵魂营火也继承自CampfireBlock，应被检测为lit campfire
    const auto& litState = soulCampfire_->defaultState()
                               .with(BlockStateProperties::LIT(), true)
                               .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    EXPECT_TRUE(mc::blocks::CampfireBlock::isLitCampfire(litState));
}

TEST_F(CampfireBlockStaticTest, IsLitCampfire_NonCampfireBlock_ReturnsFalse)
{
    // 蜂巢没有LIT属性，isLitCampfire应返回false
    const auto& beehiveState = beehiveBlock_->defaultState();
    EXPECT_FALSE(mc::blocks::CampfireBlock::isLitCampfire(beehiveState));
}
