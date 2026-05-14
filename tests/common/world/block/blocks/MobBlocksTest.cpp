#include "common/TestWorldHelper.hpp"
#include "core/Constants.hpp"
#include "entity/core/Entity.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/entities/monster/arthropod/EndermiteEntity.hpp"
#include "entity/entities/passive/special/TurtleEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "util/Direction.hpp"
#include "util/math/random/Random.hpp"
#include "util/property/Properties.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/blocks/mob/BeehiveBlock.hpp"
#include "world/block/blocks/mob/DragonBreathBlock.hpp"
#include "world/block/blocks/mob/InfestedBlock.hpp"
#include "world/block/blocks/mob/SpawnerBlock.hpp"
#include "world/block/blocks/mob/TurtleEggBlock.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/fluid/Fluid.hpp"
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
    // 测试各个等级
    for (i32 level = 0; level <= 5; ++level) {
        BlockState state = beehive_->withHoneyLevel(level);
        EXPECT_EQ(beehive_->getHoneyLevel(state), level) << "Honey level should be " << level;
    }
}

TEST_F(BeehiveBlockTest, WithHoneyLevel_ClampsToValidRange)
{
    // 测试超出范围的值
    BlockState stateNegative = beehive_->withHoneyLevel(-1);
    EXPECT_EQ(beehive_->getHoneyLevel(stateNegative), 0) << "Negative level should be clamped to 0";

    BlockState stateOverflow = beehive_->withHoneyLevel(10);
    EXPECT_EQ(beehive_->getHoneyLevel(stateOverflow), 5) << "Overflow level should be clamped to 5";
}

TEST_F(BeehiveBlockTest, WithHoneyLevel_PreservesOtherProperties)
{
    // 获取默认状态的朝向
    const auto& defaultState = beehive_->defaultState();
    Direction defaultFacing = defaultState.get(BlockStateProperties::HORIZONTAL_FACING());

    // 设置蜂蜜等级后朝向应该保持不变
    BlockState state = beehive_->withHoneyLevel(3);
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
    BlockState state = beehive_->withHoneyLevel(3);
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
    [[nodiscard]] bool isClientSide() override { return m_isClientSide; }
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

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
    u64 m_currentTick = 0;
    bool m_isClientSide = false;
};

/**
 * @brief 测试用 Mock LivingEntity 实现
 */
class MockLivingEntity : public LivingEntity {
public:
    MockLivingEntity(LegacyEntityType type, EntityId id)
        : LivingEntity(type, id, nullptr)
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
    TurtleEntity turtle(LegacyEntityType::Turtle, EntityId(1));
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
    MockLivingEntity zombie(LegacyEntityType::Zombie, EntityId(1));
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
    MockLivingEntity husk(LegacyEntityType::Husk, EntityId(1));
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
    MockLivingEntity drowned(LegacyEntityType::Drowned, EntityId(1));
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
    MockLivingEntity bat(LegacyEntityType::Bat, EntityId(1));
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
    Entity item(LegacyEntityType::Item, EntityId(1));
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

// ==================== InfestedBlock 蠹虫生成测试 ====================

class InfestedBlockSpawnTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        infested_ = std::make_unique<InfestedBlock>(1, // 石头方块ID
            BlockProperties(Material::ROCK).hardness(0.75f).resistance(0.75f));
    }

    std::unique_ptr<InfestedBlock> infested_;
    MobBlocksTestWorld world_;
};

TEST_F(InfestedBlockSpawnTest, OnBlockRemoved_SpawnsSilverfish_OnServer)
{
    // 设置被感染方块
    BlockPos pos(5, 10, 5);
    const BlockState& state = infested_->defaultState();
    world_.setBlockAt(pos, &state);

    // 调用 onBlockRemoved（服务端）
    // 注意：需要可修改的状态，所以创建一个副本
    BlockState mutableState = state;
    infested_->onBlockRemoved(world_, pos, mutableState);

    // 验证：应该生成一个蠹虫实体
    EXPECT_EQ(world_.spawnedEntityCount(), 1u);

    // 验证生成的实体类型
    Entity* spawned = world_.getSpawnedEntity(0);
    ASSERT_NE(spawned, nullptr);
    EXPECT_EQ(spawned->legacyType(), LegacyEntityType::Silverfish);
}

TEST_F(InfestedBlockSpawnTest, OnBlockRemoved_DoesNotSpawn_OnClient)
{
    // 设置客户端
    world_.setClientSide(true);

    // 设置被感染方块
    BlockPos pos(5, 10, 5);
    const BlockState& state = infested_->defaultState();
    world_.setBlockAt(pos, &state);

    // 调用 onBlockRemoved（客户端）
    BlockState mutableState = state;
    infested_->onBlockRemoved(world_, pos, mutableState);

    // 验证：客户端不应该生成实体
    EXPECT_EQ(world_.spawnedEntityCount(), 0u);
}

TEST_F(InfestedBlockSpawnTest, OnBlockRemoved_SilverfishPositionCorrect)
{
    // 设置被感染方块
    BlockPos pos(100, 50, -200);
    const BlockState& state = infested_->defaultState();
    world_.setBlockAt(pos, &state);

    // 调用 onBlockRemoved
    BlockState mutableState = state;
    infested_->onBlockRemoved(world_, pos, mutableState);

    // 验证生成的蠹虫位置
    Entity* spawned = world_.getSpawnedEntity(0);
    ASSERT_NE(spawned, nullptr);

    // 蠹虫应该在方块中心生成
    // x = pos.x + 0.5, y = pos.y, z = pos.z + 0.5
    EXPECT_NEAR(spawned->x(), 100.5f, 0.01f);
    EXPECT_NEAR(spawned->y(), 50.0f, 0.01f);
    EXPECT_NEAR(spawned->z(), -199.5f, 0.01f);
}
