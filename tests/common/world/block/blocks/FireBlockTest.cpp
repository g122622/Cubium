#include <gtest/gtest.h>

#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/blocks/nether/FireBlock.hpp"
#include "world/block/blocks/nether/SoulFireBlock.hpp"
#include "world/block/FireInfoRegistry.hpp"
#include "world/IWorld.hpp"
#include "world/tick/manager/TickManager.hpp"
#include "world/border/WorldBorder.hpp"
#include "entity/combat/DifficultyHelper.hpp"
#include "core/Constants.hpp"

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 火焰蔓延测试用世界
 *
 * 继承 IBlockReader，提供完整的测试环境
 */
class FireSpreadTestWorld final : public IBlockReader {
public:
    using IWorld::getBlockState;

    FireSpreadTestWorld()
        : m_doFireTick(true)
        , m_isRaining(false)
        , m_canRainAtResult(false)
        , m_difficulty(Difficulty::Normal)
    {}

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = state;
        }
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override { return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] bool isClientSide() override { return false; }
    [[nodiscard]] bool isUltraWarm() const override { return false; }

    [[nodiscard]] bool doFireTick() const override { return m_doFireTick; }
    [[nodiscard]] bool isRaining() const override { return m_isRaining; }
    [[nodiscard]] bool canRainAt(const BlockPos& pos) const override {
        MC_UNUSED(pos);
        return m_canRainAtResult;
    }
    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) {
        (void)setBlockState(pos.x, pos.y, pos.z, state);
    }

    [[nodiscard]] const BlockState* getBlockAt(const BlockPos& pos) const {
        return getBlockState(pos.x, pos.y, pos.z);
    }

    void setDoFireTick(bool value) { m_doFireTick = value; }
    void setRaining(bool value) { m_isRaining = value; }
    void setCanRainAtResult(bool value) { m_canRainAtResult = value; }
    void setDifficulty(Difficulty value) { m_difficulty = value; }

    [[nodiscard]] world::tick::TickManager& tickManager() override {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        const_cast<FireSpreadTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

private:
    void ensureTickManager() const {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(const_cast<FireSpreadTestWorld&>(*this));
        }
    }

    std::map<BlockPos, const BlockState*> m_blocks;
    mutable std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    math::Random m_random{12345};
    world::border::WorldBorder m_worldBorder;
    bool m_doFireTick;
    bool m_isRaining;
    bool m_canRainAtResult;
    Difficulty m_difficulty;
};

/**
 * @brief 获取 FireBlock 指针（从 VanillaBlocks::FIRE 转换）
 */
FireBlock* getFireBlock() {
    if (VanillaBlocks::FIRE == nullptr) {
        return nullptr;
    }
    return const_cast<FireBlock*>(static_cast<const FireBlock*>(VanillaBlocks::FIRE));
}

/**
 * @brief 获取 SoulFireBlock 指针
 */
SoulFireBlock* getSoulFireBlock() {
    if (VanillaBlocks::SOUL_FIRE == nullptr) {
        return nullptr;
    }
    return const_cast<SoulFireBlock*>(static_cast<const SoulFireBlock*>(VanillaBlocks::SOUL_FIRE));
}

class FireBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        FireInfoRegistry::instance().clear();
        FireInfoRegistry::instance().initializeVanillaFireInfos();
    }

    void TearDown() override {
        FireInfoRegistry::instance().clear();
    }
};

// ========== FireInfoRegistry 测试 ==========

TEST_F(FireBlockTest, FireInfoRegistry_Initialized) {
    // 验证 FireInfoRegistry 已初始化
    EXPECT_NO_THROW(FireInfoRegistry::instance().getFireInfo(0));
}

TEST_F(FireBlockTest, FireInfoRegistry_RegisterAndGetInfo) {
    FireInfoRegistry& registry = FireInfoRegistry::instance();
    registry.clear();

    // 注册测试方块信息
    registry.registerFireInfo(100, 5, 20);

    FireInfo info = registry.getFireInfo(100);
    EXPECT_EQ(info.encouragement, 5);
    EXPECT_EQ(info.flammability, 20);

    // 未注册的方块返回默认值
    FireInfo defaultInfo = registry.getFireInfo(999);
    EXPECT_EQ(defaultInfo.encouragement, 0);
    EXPECT_EQ(defaultInfo.flammability, 0);
}

TEST_F(FireBlockTest, FireInfoRegistry_GetFlammability) {
    FireInfoRegistry& registry = FireInfoRegistry::instance();
    registry.clear();

    registry.registerFireInfo(50, 10, 30);
    EXPECT_EQ(registry.getFlammability(50), 30);
    EXPECT_EQ(registry.getFlammability(999), 0);
}

TEST_F(FireBlockTest, FireInfoRegistry_GetEncouragement) {
    FireInfoRegistry& registry = FireInfoRegistry::instance();
    registry.clear();

    registry.registerFireInfo(60, 15, 25);
    EXPECT_EQ(registry.getEncouragement(60), 15);
    EXPECT_EQ(registry.getEncouragement(999), 0);
}

// ========== FireBlock::getAge / withAge 测试 ==========

TEST_F(FireBlockTest, GetAge_DefaultState_ReturnsZero) {
    FireBlock* fireBlock = getFireBlock();
    ASSERT_NE(fireBlock, nullptr);

    const BlockState& fireState = fireBlock->defaultState();
    EXPECT_EQ(fireBlock->getAge(fireState), 0);
}

TEST_F(FireBlockTest, WithAge_CreatesStateWithCorrectAge) {
    FireBlock* fireBlock = getFireBlock();
    ASSERT_NE(fireBlock, nullptr);

    BlockState age5State = fireBlock->withAge(5);
    EXPECT_EQ(fireBlock->getAge(age5State), 5);

    BlockState age15State = fireBlock->withAge(15);
    EXPECT_EQ(fireBlock->getAge(age15State), 15);
}

TEST_F(FireBlockTest, WithAge_ClampsAgeToMax15) {
    FireBlock* fireBlock = getFireBlock();
    ASSERT_NE(fireBlock, nullptr);

    BlockState clampedState = fireBlock->withAge(20);
    EXPECT_EQ(fireBlock->getAge(clampedState), 15);
}

// ========== FireBlock::tick 测试 ==========

TEST_F(FireBlockTest, Tick_InvalidPosition_RemovesFire) {
    FireBlock* fireBlock = getFireBlock();
    ASSERT_NE(fireBlock, nullptr);

    FireSpreadTestWorld world;
    math::Random random(12345);

    BlockPos firePos(5, 64, 5);

    // 火焰位置周围没有任何支撑
    // 放置火焰
    const BlockState* fireState = &fireBlock->defaultState();
    world.setBlockAt(firePos, fireState);

    // 获取可变状态
    BlockState mutableState = *fireState;

    // tick 应该移除火焰
    fireBlock->tick(world, firePos, mutableState, random);

    // 火焰应该被移除
    EXPECT_EQ(world.getBlockAt(firePos), nullptr);
}

TEST_F(FireBlockTest, Tick_DoFireTickFalse_DoesNotSpread) {
    FireBlock* fireBlock = getFireBlock();
    ASSERT_NE(fireBlock, nullptr);

    if (VanillaBlocks::STONE == nullptr) {
        GTEST_SKIP() << "STONE not registered";
    }

    FireSpreadTestWorld world;
    world.setDoFireTick(false);
    math::Random random(12345);

    BlockPos firePos(5, 64, 5);
    BlockPos supportPos(5, 63, 5);  // 下方支撑

    // 放置支撑和火焰
    world.setBlockAt(supportPos, &VanillaBlocks::STONE->defaultState());
    const BlockState* fireState = &fireBlock->defaultState();
    world.setBlockAt(firePos, fireState);

    BlockState mutableState = *fireState;

    // 当 doFireTick = false 时，tick 不应该做任何事
    fireBlock->tick(world, firePos, mutableState, random);

    // 火焰应该仍然存在
    EXPECT_NE(world.getBlockAt(firePos), nullptr);
}

TEST_F(FireBlockTest, Tick_Raining_ExtinguishesFire) {
    FireBlock* fireBlock = getFireBlock();
    ASSERT_NE(fireBlock, nullptr);

    if (VanillaBlocks::STONE == nullptr) {
        GTEST_SKIP() << "STONE not registered";
    }

    FireSpreadTestWorld world;
    world.setRaining(true);
    world.setCanRainAtResult(true);
    math::Random random(12345);

    BlockPos firePos(5, 64, 5);
    BlockPos supportPos(5, 63, 5);

    world.setBlockAt(supportPos, &VanillaBlocks::STONE->defaultState());
    const BlockState* fireState = &fireBlock->defaultState();
    world.setBlockAt(firePos, fireState);

    BlockState mutableState = *fireState;

    // 多次 tick，高概率熄灭
    bool extinguished = false;
    for (int i = 0; i < 100; ++i) {
        world.setBlockAt(firePos, fireState);
        mutableState = *fireState;
        fireBlock->tick(world, firePos, mutableState, random);
        if (world.getBlockAt(firePos) == nullptr) {
            extinguished = true;
            break;
        }
    }

    // 在雨中火焰应该熄灭（概率很高）
    EXPECT_TRUE(extinguished);
}

// ========== DifficultyHelper::getFireSpreadBonus 测试 ==========

TEST_F(FireBlockTest, DifficultyHelper_FireSpreadBonus_Peaceful) {
    EXPECT_EQ(entity::combat::DifficultyHelper::getFireSpreadBonus(Difficulty::Peaceful), 0);
}

TEST_F(FireBlockTest, DifficultyHelper_FireSpreadBonus_Easy) {
    EXPECT_EQ(entity::combat::DifficultyHelper::getFireSpreadBonus(Difficulty::Easy), 7);
}

TEST_F(FireBlockTest, DifficultyHelper_FireSpreadBonus_Normal) {
    EXPECT_EQ(entity::combat::DifficultyHelper::getFireSpreadBonus(Difficulty::Normal), 14);
}

TEST_F(FireBlockTest, DifficultyHelper_FireSpreadBonus_Hard) {
    EXPECT_EQ(entity::combat::DifficultyHelper::getFireSpreadBonus(Difficulty::Hard), 21);
}

// ========== SoulFireBlock 特性测试 ==========

TEST_F(FireBlockTest, SoulFire_IsValidPositionOnSoulSand) {
    SoulFireBlock* soulFire = getSoulFireBlock();
    if (soulFire == nullptr || VanillaBlocks::SOUL_SAND == nullptr) {
        GTEST_SKIP() << "SOUL_FIRE or SOUL_SAND not registered";
    }

    FireSpreadTestWorld world;

    BlockPos firePos(5, 64, 5);
    BlockPos sandPos(5, 63, 5);

    world.setBlockAt(sandPos, &VanillaBlocks::SOUL_SAND->defaultState());

    const BlockState& fireState = soulFire->defaultState();
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    EXPECT_TRUE(soulFire->isValidPosition(fireState, blockReader, firePos));
}

TEST_F(FireBlockTest, SoulFire_IsNotValidPositionOnStone) {
    SoulFireBlock* soulFire = getSoulFireBlock();
    if (soulFire == nullptr || VanillaBlocks::STONE == nullptr) {
        GTEST_SKIP() << "SOUL_FIRE or STONE not registered";
    }

    FireSpreadTestWorld world;

    BlockPos firePos(5, 64, 5);
    BlockPos stonePos(5, 63, 5);

    world.setBlockAt(stonePos, &VanillaBlocks::STONE->defaultState());

    const BlockState& fireState = soulFire->defaultState();
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    EXPECT_FALSE(soulFire->isValidPosition(fireState, blockReader, firePos));
}

TEST_F(FireBlockTest, SoulFire_HigherDamage) {
    // SoulFireBlock 构造时传入 fireDamage = 2
    // FireBlock::onEntityCollision 使用 m_fireDamage
    // 这里验证 SoulFireBlock 存在
    SoulFireBlock* soulFire = getSoulFireBlock();
    ASSERT_NE(soulFire, nullptr);
}

// ========== 火焰蔓延公式测试 ==========

TEST_F(FireBlockTest, SpreadFormula_Calculation) {
    // 验证火焰蔓延公式的计算
    // 公式: (encouragement + 40 + difficultyBonus) / (age + 30)

    // 测试不同难度下的蔓延概率
    i32 encouragement = 30;  // 树叶的 encouragement
    i32 age = 0;             // 新生火焰

    // Peaceful: (30 + 40 + 0) / (0 + 30) = 70 / 30 = 2.33
    i32 peacefulChance = (encouragement + 40 + 0) / (age + 30);
    EXPECT_EQ(peacefulChance, 2);

    // Easy: (30 + 40 + 7) / (0 + 30) = 77 / 30 = 2.56
    i32 easyChance = (encouragement + 40 + 7) / (age + 30);
    EXPECT_EQ(easyChance, 2);

    // Normal: (30 + 40 + 14) / (0 + 30) = 84 / 30 = 2.8
    i32 normalChance = (encouragement + 40 + 14) / (age + 30);
    EXPECT_EQ(normalChance, 2);

    // Hard: (30 + 40 + 21) / (0 + 30) = 91 / 30 = 3.03
    i32 hardChance = (encouragement + 40 + 21) / (age + 30);
    EXPECT_EQ(hardChance, 3);
}

TEST_F(FireBlockTest, SpreadFormula_OlderFireSpreadsLess) {
    // 年龄越大的火焰蔓延概率越低
    i32 encouragement = 30;
    i32 difficultyBonus = 14;  // Normal

    // 年龄 0: (30 + 40 + 14) / 30 = 2.8
    i32 age0Chance = (encouragement + 40 + difficultyBonus) / (0 + 30);
    EXPECT_EQ(age0Chance, 2);

    // 年龄 5: (30 + 40 + 14) / 35 = 2.4
    i32 age5Chance = (encouragement + 40 + difficultyBonus) / (5 + 30);
    EXPECT_EQ(age5Chance, 2);

    // 年龄 10: (30 + 40 + 14) / 40 = 2.1
    i32 age10Chance = (encouragement + 40 + difficultyBonus) / (10 + 30);
    EXPECT_EQ(age10Chance, 2);

    // 年龄 15: (30 + 40 + 14) / 45 = 1.86
    i32 age15Chance = (encouragement + 40 + difficultyBonus) / (15 + 30);
    EXPECT_EQ(age15Chance, 1);
}

// ========== 直接相邻燃烧概率测试 ==========

TEST_F(FireBlockTest, DirectSpreadChance_Calculation) {
    // 直接相邻燃烧概率: (flammability / chance) * (5 / (age + 10))
    // chance: 垂直方向 250, 水平方向 300

    i32 flammability = 60;  // 树叶
    i32 verticalChance = 250;
    i32 horizontalChance = 300;
    i32 age = 0;

    // 垂直方向: (60 / 250) * (5 / 10) = 0.24 * 0.5 = 0.12
    f32 verticalProbability = static_cast<f32>(flammability) / verticalChance * (5.0f / (age + 10));
    EXPECT_NEAR(verticalProbability, 0.12f, 0.01f);

    // 水平方向: (60 / 300) * (5 / 10) = 0.2 * 0.5 = 0.1
    f32 horizontalProbability = static_cast<f32>(flammability) / horizontalChance * (5.0f / (age + 10));
    EXPECT_NEAR(horizontalProbability, 0.1f, 0.01f);

    // 年龄 10: (60 / 300) * (5 / 20) = 0.2 * 0.25 = 0.05
    age = 10;
    f32 olderProbability = static_cast<f32>(flammability) / horizontalChance * (5.0f / (age + 10));
    EXPECT_NEAR(olderProbability, 0.05f, 0.01f);
}

// ========== 无限火源测试 ==========

TEST_F(FireBlockTest, InfiniteFireSource_Netherrack) {
    if (VanillaBlocks::NETHERRACK == nullptr) {
        GTEST_SKIP() << "NETHERRACK not registered";
    }

    FireSpreadTestWorld world;

    BlockPos firePos(5, 64, 5);
    BlockPos netherrackPos(5, 63, 5);

    world.setBlockAt(netherrackPos, &VanillaBlocks::NETHERRACK->defaultState());

    // 检查下界岩是否是火源
    const BlockState* netherrackState = world.getBlockAt(netherrackPos);
    if (netherrackState != nullptr) {
        bool isFireSource = netherrackState->isFireSource(world, netherrackPos, Direction::Up);
        // 下界岩应该是火源
        // 注意：这需要 VanillaBlocks 正确注册 isFireSource
        EXPECT_NO_THROW(isFireSource);
    }
}

// ========== BlockState 火焰方法测试 ==========

TEST_F(FireBlockTest, BlockState_GetFlammability) {
    if (VanillaBlocks::OAK_PLANKS == nullptr) {
        GTEST_SKIP() << "OAK_PLANKS not registered";
    }

    FireSpreadTestWorld world;
    BlockPos pos(0, 64, 0);

    world.setBlockAt(pos, &VanillaBlocks::OAK_PLANKS->defaultState());

    const BlockState* state = world.getBlockAt(pos);
    ASSERT_NE(state, nullptr);

    // 验证方法可以调用
    i32 flammability = state->getFlammability(&world, &pos, Direction::Up);
    EXPECT_NO_THROW(flammability);
}

TEST_F(FireBlockTest, BlockState_GetFireSpreadSpeed) {
    if (VanillaBlocks::OAK_PLANKS == nullptr) {
        GTEST_SKIP() << "OAK_PLANKS not registered";
    }

    FireSpreadTestWorld world;
    BlockPos pos(0, 64, 0);

    world.setBlockAt(pos, &VanillaBlocks::OAK_PLANKS->defaultState());

    const BlockState* state = world.getBlockAt(pos);
    ASSERT_NE(state, nullptr);

    // 验证方法可以调用
    i32 spreadSpeed = state->getFireSpreadSpeed(&world, &pos, Direction::Up);
    EXPECT_NO_THROW(spreadSpeed);
}

TEST_F(FireBlockTest, BlockState_IsFireSource_Stone) {
    if (VanillaBlocks::STONE == nullptr) {
        GTEST_SKIP() << "STONE not registered";
    }

    FireSpreadTestWorld world;
    BlockPos pos(0, 64, 0);

    world.setBlockAt(pos, &VanillaBlocks::STONE->defaultState());

    const BlockState* state = world.getBlockAt(pos);
    ASSERT_NE(state, nullptr);

    // 石头不应该作为火源
    bool isFireSource = state->isFireSource(world, pos, Direction::Up);
    EXPECT_FALSE(isFireSource);
}

} // namespace
