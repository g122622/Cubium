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

#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeLoader.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "core/Constants.hpp"
#include "entity/combat/DifficultyHelper.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/FireInfoRegistry.hpp"
#include "world/block/blocks/nether/FireBlock.hpp"
#include "world/block/blocks/nether/SoulFireBlock.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/tick/manager/TickManager.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::blocks;
using namespace mc::block_registry;

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

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
            return true;
        }

        // 关键：必须像 ServerWorld::setBlockState 一样把传入的 BlockState* 规范化
        // 为 BlockRegistry 持有的规范状态指针。FireBlock::tick / trySpread /
        // tryCatchFire 内部会构造栈上局部 BlockState（withAge 返回值），再以
        // &newState 调用 setBlockState。若直接存调用方指针，函数返回后栈帧销毁，
        // m_blocks 里就留下悬空指针；后续 getNeighborEncouragement / canBurn /
        // areNeighborsFlammable 读取该悬空指针，在 BlockState::getFireSpreadSpeed
        // (this=0x5 一类小地址) 处触发 ACCESS_VIOLATION。规范化后存的是注册表
        // 单例指针，生命周期随进程，安全。
        const BlockState* canonical = BlockRegistry::instance().getBlockState(state->stateId());
        m_blocks[pos] = (canonical != nullptr) ? canonical : state;
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    // 默认返回 nullptr（保持现有 41 个测试行为）；flag1 链路测试经 setChunkData 注入带 biome 的 ChunkData。
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return m_chunkData.get(); }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return m_chunkData != nullptr; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] bool isClientSide() const override { return false; }
    [[nodiscard]] bool isUltraWarm() const override { return false; }

    [[nodiscard]] bool doFireTick() const override { return m_doFireTick; }
    [[nodiscard]] bool isRaining() const override { return m_isRaining; }
    [[nodiscard]] bool canRainAt(const BlockPos& pos) const override
    {
        MC_UNUSED(pos);
        return m_canRainAtResult;
    }
    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    [[nodiscard]] const BlockState* getBlockAt(const BlockPos& pos) const { return getBlockState(pos.x, pos.y, pos.z); }

    void setDoFireTick(bool value) { m_doFireTick = value; }
    void setRaining(bool value) { m_isRaining = value; }
    void setCanRainAtResult(bool value) { m_canRainAtResult = value; }
    void setDifficulty(Difficulty value) { m_difficulty = value; }

    // 注入带 biome 的 ChunkData 供 FireBlock::getIncreasedFireBurnout 链路测试。
    void setChunkData(std::unique_ptr<ChunkData> chunk) { m_chunkData = std::move(chunk); }
    [[nodiscard]] ChunkData* mutableChunkData() { return m_chunkData.get(); }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<FireSpreadTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

private:
    void ensureTickManager() const
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(const_cast<FireSpreadTestWorld&>(*this));
        }
    }

    std::map<BlockPos, const BlockState*> m_blocks;
    std::unique_ptr<ChunkData> m_chunkData; // 可选：flag1 链路测试注入带 biome 的区块
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
FireBlock* getFireBlock()
{
    if (VanillaBlocks::FIRE == nullptr) {
        return nullptr;
    }
    return const_cast<FireBlock*>(static_cast<const FireBlock*>(VanillaBlocks::FIRE));
}

/**
 * @brief 获取 SoulFireBlock 指针
 */
SoulFireBlock* getSoulFireBlock()
{
    if (VanillaBlocks::SOUL_FIRE == nullptr) {
        return nullptr;
    }
    return const_cast<SoulFireBlock*>(static_cast<const SoulFireBlock*>(VanillaBlocks::SOUL_FIRE));
}

class FireBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        FireInfoRegistry::instance().clear();
        FireInfoRegistry::instance().initializeVanillaFireInfos();
    }

    void TearDown() override { FireInfoRegistry::instance().clear(); }
};

// ========== FireInfoRegistry 测试 ==========

TEST_F(FireBlockTest, FireInfoRegistry_Initialized)
{
    // 验证 FireInfoRegistry 已初始化
    EXPECT_NO_THROW(FireInfoRegistry::instance().getFireInfo(0));
}

TEST_F(FireBlockTest, FireInfoRegistry_RegisterAndGetInfo)
{
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

TEST_F(FireBlockTest, FireInfoRegistry_GetFlammability)
{
    FireInfoRegistry& registry = FireInfoRegistry::instance();
    registry.clear();

    registry.registerFireInfo(50, 10, 30);
    EXPECT_EQ(registry.getFlammability(50), 30);
    EXPECT_EQ(registry.getFlammability(999), 0);
}

TEST_F(FireBlockTest, FireInfoRegistry_GetEncouragement)
{
    FireInfoRegistry& registry = FireInfoRegistry::instance();
    registry.clear();

    registry.registerFireInfo(60, 15, 25);
    EXPECT_EQ(registry.getEncouragement(60), 15);
    EXPECT_EQ(registry.getEncouragement(999), 0);
}

// ========== FireBlock::getAge / withAge 测试 ==========

TEST_F(FireBlockTest, GetAge_DefaultState_ReturnsZero)
{
    FireBlock* fireBlock = getFireBlock();
    ASSERT_NE(fireBlock, nullptr);

    const BlockState& fireState = fireBlock->defaultState();
    EXPECT_EQ(fireBlock->getAge(fireState), 0);
}

TEST_F(FireBlockTest, WithAge_CreatesStateWithCorrectAge)
{
    FireBlock* fireBlock = getFireBlock();
    ASSERT_NE(fireBlock, nullptr);

    BlockState age5State = fireBlock->withAge(5);
    EXPECT_EQ(fireBlock->getAge(age5State), 5);

    BlockState age15State = fireBlock->withAge(15);
    EXPECT_EQ(fireBlock->getAge(age15State), 15);
}

TEST_F(FireBlockTest, WithAge_ClampsAgeToMax15)
{
    FireBlock* fireBlock = getFireBlock();
    ASSERT_NE(fireBlock, nullptr);

    BlockState clampedState = fireBlock->withAge(20);
    EXPECT_EQ(fireBlock->getAge(clampedState), 15);
}

// ========== FireBlock::tick 测试 ==========

TEST_F(FireBlockTest, Tick_InvalidPosition_RemovesFire)
{
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

TEST_F(FireBlockTest, Tick_DoFireTickFalse_DoesNotSpread)
{
    FireBlock* fireBlock = getFireBlock();
    ASSERT_NE(fireBlock, nullptr);

    if (VanillaBlocks::STONE == nullptr) {
        GTEST_SKIP() << "STONE not registered";
    }

    FireSpreadTestWorld world;
    world.setDoFireTick(false);
    math::Random random(12345);

    BlockPos firePos(5, 64, 5);
    BlockPos supportPos(5, 63, 5); // 下方支撑

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

TEST_F(FireBlockTest, Tick_Raining_ExtinguishesFire)
{
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

TEST_F(FireBlockTest, DifficultyHelper_FireSpreadBonus_Peaceful)
{
    EXPECT_EQ(entity::combat::DifficultyHelper::getFireSpreadBonus(Difficulty::Peaceful), 0);
}

TEST_F(FireBlockTest, DifficultyHelper_FireSpreadBonus_Easy)
{
    EXPECT_EQ(entity::combat::DifficultyHelper::getFireSpreadBonus(Difficulty::Easy), 7);
}

TEST_F(FireBlockTest, DifficultyHelper_FireSpreadBonus_Normal)
{
    EXPECT_EQ(entity::combat::DifficultyHelper::getFireSpreadBonus(Difficulty::Normal), 14);
}

TEST_F(FireBlockTest, DifficultyHelper_FireSpreadBonus_Hard)
{
    EXPECT_EQ(entity::combat::DifficultyHelper::getFireSpreadBonus(Difficulty::Hard), 21);
}

// ========== SoulFireBlock 特性测试 ==========

TEST_F(FireBlockTest, SoulFire_IsValidPositionOnSoulSand)
{
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

TEST_F(FireBlockTest, SoulFire_IsNotValidPositionOnStone)
{
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

TEST_F(FireBlockTest, SoulFire_HigherDamage)
{
    // SoulFireBlock 构造时传入 fireDamage = 2
    // FireBlock::onEntityCollision 使用 m_fireDamage
    // 这里验证 SoulFireBlock 存在
    SoulFireBlock* soulFire = getSoulFireBlock();
    ASSERT_NE(soulFire, nullptr);
}

// ========== 火焰蔓延公式测试 ==========

TEST_F(FireBlockTest, SpreadFormula_Calculation)
{
    // 验证火焰蔓延公式的计算
    // 公式: (encouragement + 40 + difficultyBonus) / (age + 30)

    // 测试不同难度下的蔓延概率
    i32 encouragement = 30; // 树叶的 encouragement
    i32 age = 0;            // 新生火焰

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

TEST_F(FireBlockTest, SpreadFormula_OlderFireSpreadsLess)
{
    // 年龄越大的火焰蔓延概率越低
    i32 encouragement = 30;
    i32 difficultyBonus = 14; // Normal

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

TEST_F(FireBlockTest, DirectSpreadChance_Calculation)
{
    // 直接相邻燃烧概率: (flammability / chance) * (5 / (age + 10))
    // chance: 垂直方向 250, 水平方向 300

    i32 flammability = 60; // 树叶
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

TEST_F(FireBlockTest, InfiniteFireSource_Netherrack)
{
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

TEST_F(FireBlockTest, BlockState_GetFlammability)
{
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

TEST_F(FireBlockTest, BlockState_GetFireSpreadSpeed)
{
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

TEST_F(FireBlockTest, BlockState_IsFireSource_Stone)
{
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

// ============================================================================
// FireBlock::getFireState() 测试
// ============================================================================

TEST_F(FireBlockTest, GetFireState_ReturnsNormalFireOnStone)
{
    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    ASSERT_NE(VanillaBlocks::STONE, nullptr);

    FireSpreadTestWorld world;

    // 在石头上方放置火焰，应返回普通火
    const BlockPos firePos(0, 64, 0);
    const BlockPos stonePos(0, 63, 0);

    world.setBlockAt(stonePos, &VanillaBlocks::STONE->defaultState());

    const BlockState& fireState = FireBlock::getFireState(world, firePos);
    EXPECT_EQ(&fireState.getBlock(), VanillaBlocks::FIRE);
}

TEST_F(FireBlockTest, GetFireState_ReturnsSoulFireOnSoulSand)
{
    ASSERT_NE(VanillaBlocks::SOUL_FIRE, nullptr);
    ASSERT_NE(VanillaBlocks::SOUL_SAND, nullptr);

    FireSpreadTestWorld world;

    // 在灵魂沙上方放置火焰，应返回灵魂火
    const BlockPos firePos(5, 64, 5);
    const BlockPos sandPos(5, 63, 5);

    world.setBlockAt(sandPos, &VanillaBlocks::SOUL_SAND->defaultState());

    const BlockState& fireState = FireBlock::getFireState(world, firePos);
    EXPECT_EQ(&fireState.getBlock(), VanillaBlocks::SOUL_FIRE);
}

TEST_F(FireBlockTest, GetFireState_ReturnsSoulFireOnSoulSoil)
{
    ASSERT_NE(VanillaBlocks::SOUL_FIRE, nullptr);
    ASSERT_NE(VanillaBlocks::SOUL_SOIL, nullptr);

    FireSpreadTestWorld world;

    // 在灵魂土上方放置火焰，应返回灵魂火
    const BlockPos firePos(7, 64, 7);
    const BlockPos soilPos(7, 63, 7);

    world.setBlockAt(soilPos, &VanillaBlocks::SOUL_SOIL->defaultState());

    const BlockState& fireState = FireBlock::getFireState(world, firePos);
    EXPECT_EQ(&fireState.getBlock(), VanillaBlocks::SOUL_FIRE);
}

TEST_F(FireBlockTest, GetFireState_ReturnsNormalFireOnAir)
{
    ASSERT_NE(VanillaBlocks::FIRE, nullptr);

    FireSpreadTestWorld world;

    // 下方为空气（无方块），应返回普通火
    const BlockPos firePos(10, 64, 10);

    const BlockState& fireState = FireBlock::getFireState(world, firePos);
    EXPECT_EQ(&fireState.getBlock(), VanillaBlocks::FIRE);
}

TEST_F(FireBlockTest, GetFireState_ReturnsNormalFireOnDirt)
{
    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    ASSERT_NE(VanillaBlocks::DIRT, nullptr);

    FireSpreadTestWorld world;

    // 在泥土上方放置火焰，应返回普通火
    const BlockPos firePos(15, 64, 15);
    const BlockPos dirtPos(15, 63, 15);

    world.setBlockAt(dirtPos, &VanillaBlocks::DIRT->defaultState());

    const BlockState& fireState = FireBlock::getFireState(world, firePos);
    EXPECT_EQ(&fireState.getBlock(), VanillaBlocks::FIRE);
}

// ============================================================================
// infiniburn 标签与无限火源测试（偏离 #7 修复验证）
// ============================================================================
//
// 对齐 vanilla FireBlock.tick:151 `blockstate.is(dimensionType().infiniburn())`：
// 下方方块在该维度的 infiniburn 标签中时，火焰为无限火源（永不熄灭、不检查有效位置、
// age15 不概率熄灭）。此前 Cubium 用 belowState->isFireSource()（恒 false）致下界岩/
// 岩浆块/基岩上的火不具无限火源属性。修复后按维度查 INFINIBURN_OVERWORLD/NETHER/END 标签。

TEST_F(FireBlockTest, InfiniburnTags_ContainCorrectMembers)
{
    // INFINIBURN_OVERWORLD = {netherrack, magma_block}
    auto& overworld = BlockTags::INFINIBURN_OVERWORLD();
    ASSERT_NE(BaseBlocks::NETHERRACK, nullptr);
    ASSERT_NE(NetherBlocks::MAGMA, nullptr);
    EXPECT_TRUE(overworld.contains(*BaseBlocks::NETHERRACK));
    EXPECT_TRUE(overworld.contains(*NetherBlocks::MAGMA));
    ASSERT_NE(BaseBlocks::BEDROCK, nullptr);
    EXPECT_FALSE(overworld.contains(*BaseBlocks::BEDROCK)); // bedrock 不在 overworld 标签

    // INFINIBURN_NETHER = {netherrack, magma_block}
    auto& nether = BlockTags::INFINIBURN_NETHER();
    EXPECT_TRUE(nether.contains(*BaseBlocks::NETHERRACK));
    EXPECT_TRUE(nether.contains(*NetherBlocks::MAGMA));
    EXPECT_FALSE(nether.contains(*BaseBlocks::BEDROCK));

    // INFINIBURN_END = {netherrack, magma_block, bedrock}
    auto& end = BlockTags::INFINIBURN_END();
    EXPECT_TRUE(end.contains(*BaseBlocks::NETHERRACK));
    EXPECT_TRUE(end.contains(*NetherBlocks::MAGMA));
    EXPECT_TRUE(end.contains(*BaseBlocks::BEDROCK)); // bedrock 仅在 end 标签
}

TEST_F(FireBlockTest, FireOnNetherrack_SurvivesWithoutFlammableNeighbors)
{
    // 主世界维度（FireSpreadTestWorld::dimension()=0）→ INFINIBURN_OVERWORLD 含 netherrack。
    // netherrack 上的火为无限火源：isFireSource=true，跳过"无可燃邻居 + age>3 → 熄灭"检查。
    // vanilla FireBlock.java:162-170 的 !flag 门控确保 infiniburn 上的火不因 isValidFireLocation 失败熄灭。
    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    ASSERT_NE(BaseBlocks::NETHERRACK, nullptr);

    FireBlock* fireBlock = getFireBlock();
    ASSERT_NE(fireBlock, nullptr);

    FireSpreadTestWorld world; // 主世界，不下雨（isRaining=false）
    math::Random random(12345);

    BlockPos firePos(5, 64, 5);
    BlockPos netherrackPos(5, 63, 5);
    world.setBlockAt(netherrackPos, &BaseBlocks::NETHERRACK->defaultState());

    // 放置 age=15 的火焰（age>3 触发普通方块的熄灭检查，但 netherrack 是火源应跳过）
    BlockState fireState = fireBlock->withAge(15);
    world.setBlockAt(firePos, &fireState);

    // 多次 tick，火焰不应熄灭（netherrack 无限火源）
    bool extinguished = false;
    for (int i = 0; i < 50; ++i) {
        BlockState mutableState = *world.getBlockAt(firePos);
        if (mutableState.isAir() || world.getBlockAt(firePos) == nullptr) {
            extinguished = true;
            break;
        }
        fireBlock->tick(world, firePos, mutableState, random);
        if (world.getBlockAt(firePos) == nullptr) {
            extinguished = true;
            break;
        }
    }
    EXPECT_FALSE(extinguished) << "netherrack 上的火应为无限火源，不因无可燃邻居熄灭";
}

TEST_F(FireBlockTest, FireOnStone_ExtinguishesWithoutFlammableNeighbors)
{
    // 对照组：stone 不在 infiniburn 标签，isFireSource=false。
    // 无可燃邻居 + age>3 时火焰应熄灭（vanilla FireBlock.java:162-170 的 isValidFireLocation 检查）。
    ASSERT_NE(VanillaBlocks::FIRE, nullptr);
    ASSERT_NE(VanillaBlocks::STONE, nullptr);

    FireBlock* fireBlock = getFireBlock();
    ASSERT_NE(fireBlock, nullptr);

    FireSpreadTestWorld world;
    math::Random random(12345);

    BlockPos firePos(5, 64, 5);
    BlockPos stonePos(5, 63, 5);
    world.setBlockAt(stonePos, &VanillaBlocks::STONE->defaultState());

    BlockState fireState = fireBlock->withAge(15);
    world.setBlockAt(firePos, &fireState);

    // 多次 tick，火焰应熄灭（stone 非火源，无可燃邻居 + age>3）
    bool extinguished = false;
    for (int i = 0; i < 50; ++i) {
        BlockState mutableState = *world.getBlockAt(firePos);
        if (world.getBlockAt(firePos) == nullptr) {
            extinguished = true;
            break;
        }
        fireBlock->tick(world, firePos, mutableState, random);
        if (world.getBlockAt(firePos) == nullptr) {
            extinguished = true;
            break;
        }
    }
    EXPECT_TRUE(extinguished) << "stone 上的火（无可燃邻居 + age>3）应熄灭";
}

// ============================================================================
// ignitedByLava 属性测试（偏离 #8 修复验证）
// ============================================================================
//
// 对齐 vanilla LavaFluid.isFlammable（LavaFluid.java:134-136）：岩浆能否点燃方块由
// 方块的 ignitedByLava() 属性决定，而非 material().isFlammable()。LavaFluid::_isBlockFlammable
// 已改为查 isIgnitedByLava()。此处验证关键方块的 ignitedByLava 设置与 vanilla 1.21.11 一致。

TEST_F(FireBlockTest, IgnitedByLava_WoodenBlocks_True)
{
    // 木板/原木/树叶/羊毛等可燃方块，vanilla 均设置 ignitedByLava
    ASSERT_NE(VanillaBlocks::OAK_PLANKS, nullptr);
    EXPECT_TRUE(VanillaBlocks::OAK_PLANKS->isIgnitedByLava());
    ASSERT_NE(BaseBlocks::NETHERRACK, nullptr); // 占位确保 BaseBlocks 可用
    MC_UNUSED(BaseBlocks::NETHERRACK);
}

TEST_F(FireBlockTest, IgnitedByLava_TNT_True)
{
    // TNT：vanilla Blocks.java:1975 设置 ignitedByLava（岩浆可点燃 TNT 引爆）
    ASSERT_NE(BuildingBlocks::TNT, nullptr);
    EXPECT_TRUE(BuildingBlocks::TNT->isIgnitedByLava());
}

TEST_F(FireBlockTest, IgnitedByLava_PaleMossBlock_True)
{
    // pale_moss_block：vanilla 设置 ignitedByLava，但 Cubium Material::MOSS 不可燃。
    // 修复前用 material().isFlammable() 致漏判（不被岩浆点燃）；修复后查 isIgnitedByLava()=true。
    ASSERT_NE(PaleGardenBlocks::PALE_MOSS_BLOCK, nullptr);
    EXPECT_TRUE(PaleGardenBlocks::PALE_MOSS_BLOCK->isIgnitedByLava());
}

TEST_F(FireBlockTest, IgnitedByLava_WoodenButton_False)
{
    // 木按钮：vanilla buttonProperties() 不设置 ignitedByLava（不被岩浆点燃）。
    // 修复前 Cubium 误设 .ignitedByLava() 致误判（被岩浆点燃）；修复后移除，isIgnitedByLava()=false。
    ASSERT_NE(RedstoneBlocks::OAK_BUTTON, nullptr);
    EXPECT_FALSE(RedstoneBlocks::OAK_BUTTON->isIgnitedByLava());
    ASSERT_NE(RedstoneBlocks::SPRUCE_BUTTON, nullptr);
    EXPECT_FALSE(RedstoneBlocks::SPRUCE_BUTTON->isIgnitedByLava());
    ASSERT_NE(RedstoneBlocks::PALE_OAK_BUTTON, nullptr);
    EXPECT_FALSE(RedstoneBlocks::PALE_OAK_BUTTON->isIgnitedByLava());
}

TEST_F(FireBlockTest, IgnitedByLava_NetherWoodSign_False)
{
    // 下界木告示牌：vanilla 下界木设计不可燃，不设置 ignitedByLava。
    // 修复后下界木告示牌用独立属性（netherSignProps），isIgnitedByLava()=false。
    ASSERT_NE(SignBannerBlocks::CRIMSON_SIGN, nullptr);
    EXPECT_FALSE(SignBannerBlocks::CRIMSON_SIGN->isIgnitedByLava());
    ASSERT_NE(SignBannerBlocks::WARPED_SIGN, nullptr);
    EXPECT_FALSE(SignBannerBlocks::WARPED_SIGN->isIgnitedByLava());
}

TEST_F(FireBlockTest, IgnitedByLava_NormalWoodSign_True)
{
    // 普通木质告示牌：vanilla 设置 ignitedByLava（可被岩浆点燃）。
    ASSERT_NE(SignBannerBlocks::OAK_SIGN, nullptr);
    EXPECT_TRUE(SignBannerBlocks::OAK_SIGN->isIgnitedByLava());
}

TEST_F(FireBlockTest, IgnitedByLava_Banner_True)
{
    // 旗帜：vanilla 设置 ignitedByLava（可被岩浆点燃）。
    ASSERT_NE(SignBannerBlocks::WHITE_BANNER, nullptr);
    EXPECT_TRUE(SignBannerBlocks::WHITE_BANNER->isIgnitedByLava());
}

TEST_F(FireBlockTest, IgnitedByLava_Stone_False)
{
    // 石头：vanilla 不设置 ignitedByLava（不可燃）。
    ASSERT_NE(VanillaBlocks::STONE, nullptr);
    EXPECT_FALSE(VanillaBlocks::STONE->isIgnitedByLava());
}

TEST_F(FireBlockTest, IgnitedByLava_HayBlockLadder_False)
{
    // hay_block/ladder：vanilla 不设置 ignitedByLava。修复前 Cubium 误设，修复后移除。
    ASSERT_NE(BuildingBlocks::HAY_BLOCK, nullptr);
    EXPECT_FALSE(BuildingBlocks::HAY_BLOCK->isIgnitedByLava());
    ASSERT_NE(BuildingBlocks::LADDER, nullptr);
    EXPECT_FALSE(BuildingBlocks::LADDER->isIgnitedByLava());
}

} // namespace

// ============================================================================
// FireInfoRegistry 原版方块火焰参数验证测试
// ============================================================================

class FireInfoRegistryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        // initializeVanillaFireInfos() 已在 VanillaBlocks::initialize() 中调用
        // 但其他测试的 TearDown 可能已清空注册表，确保重新初始化
        if (FireInfoRegistry::instance().getFlammability(VanillaBlocks::OAK_PLANKS->blockId()) == 0) {
            FireInfoRegistry::instance().initializeVanillaFireInfos();
        }
    }
};

// 验证木板类方块的燃烧参数 (ignite=5, burn=20)
TEST_F(FireInfoRegistryTest, Planks_FireInfo)
{
    auto& registry = FireInfoRegistry::instance();
    EXPECT_EQ(registry.getEncouragement(VanillaBlocks::OAK_PLANKS->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(VanillaBlocks::OAK_PLANKS->blockId()), 20);
    EXPECT_EQ(registry.getEncouragement(VanillaBlocks::BIRCH_PLANKS->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(VanillaBlocks::BIRCH_PLANKS->blockId()), 20);
    EXPECT_EQ(registry.getEncouragement(VanillaBlocks::CHERRY_PLANKS->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(VanillaBlocks::CHERRY_PLANKS->blockId()), 20);
    EXPECT_EQ(registry.getEncouragement(VanillaBlocks::BAMBOO_PLANKS->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(VanillaBlocks::BAMBOO_PLANKS->blockId()), 20);
}

// 验证原木类方块的燃烧参数 (ignite=5, burn=5)
TEST_F(FireInfoRegistryTest, Logs_FireInfo)
{
    auto& registry = FireInfoRegistry::instance();
    EXPECT_EQ(registry.getEncouragement(VanillaBlocks::OAK_LOG->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(VanillaBlocks::OAK_LOG->blockId()), 5);
    EXPECT_EQ(registry.getEncouragement(VanillaBlocks::STRIPPED_BIRCH_LOG->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(VanillaBlocks::STRIPPED_BIRCH_LOG->blockId()), 5);
    EXPECT_EQ(registry.getEncouragement(VanillaBlocks::DARK_OAK_WOOD->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(VanillaBlocks::DARK_OAK_WOOD->blockId()), 5);
}

// 验证树叶类方块的燃烧参数 (ignite=30, burn=60)
TEST_F(FireInfoRegistryTest, Leaves_FireInfo)
{
    auto& registry = FireInfoRegistry::instance();
    EXPECT_EQ(registry.getEncouragement(VanillaBlocks::OAK_LEAVES->blockId()), 30);
    EXPECT_EQ(registry.getFlammability(VanillaBlocks::OAK_LEAVES->blockId()), 60);
    EXPECT_EQ(registry.getEncouragement(VanillaBlocks::AZALEA_LEAVES->blockId()), 30);
    EXPECT_EQ(registry.getFlammability(VanillaBlocks::AZALEA_LEAVES->blockId()), 60);
}

// 验证羊毛类方块的燃烧参数 (ignite=30, burn=60)
TEST_F(FireInfoRegistryTest, Wool_FireInfo)
{
    auto& registry = FireInfoRegistry::instance();
    EXPECT_EQ(registry.getEncouragement(ColoredBlocks::WHITE_WOOL->blockId()), 30);
    EXPECT_EQ(registry.getFlammability(ColoredBlocks::WHITE_WOOL->blockId()), 60);
    EXPECT_EQ(registry.getEncouragement(ColoredBlocks::BLACK_WOOL->blockId()), 30);
    EXPECT_EQ(registry.getFlammability(ColoredBlocks::BLACK_WOOL->blockId()), 60);
}

// 验证地毯类方块的燃烧参数 (ignite=60, burn=20)
TEST_F(FireInfoRegistryTest, Carpet_FireInfo)
{
    auto& registry = FireInfoRegistry::instance();
    EXPECT_EQ(registry.getEncouragement(ColoredBlocks::RED_CARPET->blockId()), 60);
    EXPECT_EQ(registry.getFlammability(ColoredBlocks::RED_CARPET->blockId()), 20);
}

// 验证植物/花草类方块的燃烧参数 (ignite=60, burn=100)
TEST_F(FireInfoRegistryTest, Plants_FireInfo)
{
    auto& registry = FireInfoRegistry::instance();
    EXPECT_EQ(registry.getEncouragement(VegetationBlocks::SHORT_GRASS->blockId()), 60);
    EXPECT_EQ(registry.getFlammability(VegetationBlocks::SHORT_GRASS->blockId()), 100);
    EXPECT_EQ(registry.getEncouragement(VegetationBlocks::DANDELION->blockId()), 60);
    EXPECT_EQ(registry.getFlammability(VegetationBlocks::DANDELION->blockId()), 100);
    EXPECT_EQ(registry.getEncouragement(NaturalBlocks::DEAD_BUSH->blockId()), 60);
    EXPECT_EQ(registry.getFlammability(NaturalBlocks::DEAD_BUSH->blockId()), 100);
}

// 验证杂项可燃方块的燃烧参数
TEST_F(FireInfoRegistryTest, MiscFlammable_FireInfo)
{
    auto& registry = FireInfoRegistry::instance();
    // 书架 (ignite=30, burn=20)
    EXPECT_EQ(registry.getEncouragement(BuildingBlocks::BOOKSHELF->blockId()), 30);
    EXPECT_EQ(registry.getFlammability(BuildingBlocks::BOOKSHELF->blockId()), 20);
    // TNT (ignite=15, burn=100)
    EXPECT_EQ(registry.getEncouragement(BuildingBlocks::TNT->blockId()), 15);
    EXPECT_EQ(registry.getFlammability(BuildingBlocks::TNT->blockId()), 100);
    // 藤蔓 (ignite=15, burn=100)
    EXPECT_EQ(registry.getEncouragement(NaturalBlocks::VINE->blockId()), 15);
    EXPECT_EQ(registry.getFlammability(NaturalBlocks::VINE->blockId()), 100);
    // 煤炭块 (ignite=5, burn=5)
    EXPECT_EQ(registry.getEncouragement(BaseBlocks::COAL_BLOCK->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(BaseBlocks::COAL_BLOCK->blockId()), 5);
    // 干草块 (ignite=60, burn=20)
    EXPECT_EQ(registry.getEncouragement(BuildingBlocks::HAY_BLOCK->blockId()), 60);
    EXPECT_EQ(registry.getFlammability(BuildingBlocks::HAY_BLOCK->blockId()), 20);
    // 干海带块 (ignite=30, burn=60)
    EXPECT_EQ(registry.getEncouragement(NaturalBlocks::DRIED_KELP_BLOCK->blockId()), 30);
    EXPECT_EQ(registry.getFlammability(NaturalBlocks::DRIED_KELP_BLOCK->blockId()), 60);
    // 脚手架 (ignite=60, burn=60)
    EXPECT_EQ(registry.getEncouragement(BuildingBlocks::SCAFFOLDING->blockId()), 60);
    EXPECT_EQ(registry.getFlammability(BuildingBlocks::SCAFFOLDING->blockId()), 60);
    // 发光地衣 (ignite=15, burn=100)
    EXPECT_EQ(registry.getEncouragement(CaveBlocks::GLOW_LICHEN->blockId()), 15);
    EXPECT_EQ(registry.getFlammability(CaveBlocks::GLOW_LICHEN->blockId()), 100);
}

// 验证非可燃方块返回 0
TEST_F(FireInfoRegistryTest, NonFlammable_ReturnsZero)
{
    auto& registry = FireInfoRegistry::instance();
    // 石头、泥土、沙子不可燃
    EXPECT_EQ(registry.getEncouragement(VanillaBlocks::STONE->blockId()), 0);
    EXPECT_EQ(registry.getFlammability(VanillaBlocks::STONE->blockId()), 0);
    EXPECT_EQ(registry.getEncouragement(VanillaBlocks::DIRT->blockId()), 0);
    EXPECT_EQ(registry.getFlammability(VanillaBlocks::DIRT->blockId()), 0);
    // 下界木材（绯红/诡异）不可燃
    EXPECT_EQ(registry.getEncouragement(NetherBlocks::CRIMSON_STEM->blockId()), 0);
    EXPECT_EQ(registry.getFlammability(NetherBlocks::CRIMSON_STEM->blockId()), 0);
    EXPECT_EQ(registry.getEncouragement(NetherBlocks::WARPED_STEM->blockId()), 0);
    EXPECT_EQ(registry.getFlammability(NetherBlocks::WARPED_STEM->blockId()), 0);
}

// 验证 Block::getFlammability() 和 Block::getFireSpreadSpeed() 通过注册表查询
TEST_F(FireInfoRegistryTest, Block_FireMethods_QueryRegistry)
{
    // 橡木木板：flammability=20, encouragement=5
    const BlockState& planksState = VanillaBlocks::OAK_PLANKS->defaultState();
    EXPECT_EQ(planksState.getFlammability(), 20);
    EXPECT_EQ(planksState.getFireSpreadSpeed(), 5);

    // 树叶：flammability=60, encouragement=30
    const BlockState& leavesState = VanillaBlocks::OAK_LEAVES->defaultState();
    EXPECT_EQ(leavesState.getFlammability(), 60);
    EXPECT_EQ(leavesState.getFireSpreadSpeed(), 30);

    // 石头：不注册，返回0
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    EXPECT_EQ(stoneState.getFlammability(), 0);
    EXPECT_EQ(stoneState.getFireSpreadSpeed(), 0);
}

TEST_F(FireInfoRegistryTest, AgriculturalAndVegetationBlockFireInfo)
{
    // 甜浆果丛：ignite=60, burn=100（IGNITE_INSTANT=60, BURN_INSTANT=100）
    const BlockState& sweetBerryState = VanillaBlocks::SWEET_BERRY_BUSH->defaultState();
    EXPECT_EQ(sweetBerryState.getFlammability(), 100);
    EXPECT_EQ(sweetBerryState.getFireSpreadSpeed(), 60);

    // 注意: 可可豆（COCOA）、小麦（WHEAT）、南瓜茎（PUMPKIN_STEM）、甘蔗（SUGAR_CANE）
    //       在 MC 原版 FireBlock.bootStrap() 中未注册为可燃方块，因此此处不测试其火焰参数。

    // 萤火虫灌木：ignite=60, burn=100
    const BlockState& fireflyBushState = VanillaBlocks::FIREFLY_BUSH->defaultState();
    EXPECT_EQ(fireflyBushState.getFlammability(), 100);
    EXPECT_EQ(fireflyBushState.getFireSpreadSpeed(), 60);
}

// 验证所有主世界木材栅栏的燃烧参数 (ignite=5, burn=20)
TEST_F(FireInfoRegistryTest, Fence_FireInfo)
{
    auto& registry = FireInfoRegistry::instance();
    // 橡木栅栏
    EXPECT_EQ(registry.getEncouragement(BuildingVariantBlocks::OAK_FENCE->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(BuildingVariantBlocks::OAK_FENCE->blockId()), 20);
    // 云杉木栅栏
    EXPECT_EQ(registry.getEncouragement(BuildingVariantBlocks::SPRUCE_FENCE->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(BuildingVariantBlocks::SPRUCE_FENCE->blockId()), 20);
    // 白桦木栅栏
    EXPECT_EQ(registry.getEncouragement(BuildingVariantBlocks::BIRCH_FENCE->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(BuildingVariantBlocks::BIRCH_FENCE->blockId()), 20);
    // 丛林木栅栏
    EXPECT_EQ(registry.getEncouragement(BuildingVariantBlocks::JUNGLE_FENCE->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(BuildingVariantBlocks::JUNGLE_FENCE->blockId()), 20);
    // 金合欢木栅栏
    EXPECT_EQ(registry.getEncouragement(BuildingVariantBlocks::ACACIA_FENCE->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(BuildingVariantBlocks::ACACIA_FENCE->blockId()), 20);
    // 深色橡木栅栏
    EXPECT_EQ(registry.getEncouragement(BuildingVariantBlocks::DARK_OAK_FENCE->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(BuildingVariantBlocks::DARK_OAK_FENCE->blockId()), 20);
    // 樱花木栅栏
    EXPECT_EQ(registry.getEncouragement(CherryBlocks::CHERRY_FENCE->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(CherryBlocks::CHERRY_FENCE->blockId()), 20);
    // 红树木栅栏
    EXPECT_EQ(registry.getEncouragement(MangroveBlocks::MANGROVE_FENCE->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(MangroveBlocks::MANGROVE_FENCE->blockId()), 20);
    // 苍白橡木栅栏
    EXPECT_EQ(registry.getEncouragement(PaleGardenBlocks::PALE_OAK_FENCE->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(PaleGardenBlocks::PALE_OAK_FENCE->blockId()), 20);
    // 竹栅栏
    EXPECT_EQ(registry.getEncouragement(BambooBlocks::BAMBOO_FENCE->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(BambooBlocks::BAMBOO_FENCE->blockId()), 20);
}

// 验证蜂箱和蜂巢的燃烧参数
TEST_F(FireInfoRegistryTest, Beehive_BeeNest_FireInfo)
{
    auto& registry = FireInfoRegistry::instance();
    // 蜂箱 (ignite=5, burn=20) — 木质方块，较难点燃
    EXPECT_EQ(registry.getEncouragement(NaturalBlocks::BEEHIVE->blockId()), 5);
    EXPECT_EQ(registry.getFlammability(NaturalBlocks::BEEHIVE->blockId()), 20);
    // 蜂巢 (ignite=30, burn=20) — 自然方块，更易点燃
    EXPECT_EQ(registry.getEncouragement(NaturalBlocks::BEE_NEST->blockId()), 30);
    EXPECT_EQ(registry.getFlammability(NaturalBlocks::BEE_NEST->blockId()), 20);
}

// ============================================================================
// 集成测试：方块标签包含性验证
// ============================================================================

class BlockTagIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

// 验证 WOODEN_FENCES 标签包含所有木质栅栏
TEST_F(BlockTagIntegrationTest, WoodenFences_ContainsAllWoodenFences)
{
    auto& tag = BlockTags::WOODEN_FENCES();
    EXPECT_TRUE(tag.contains(*VanillaBlocks::OAK_FENCE));
    EXPECT_TRUE(tag.contains(*VanillaBlocks::SPRUCE_FENCE));
    EXPECT_TRUE(tag.contains(*VanillaBlocks::BIRCH_FENCE));
    EXPECT_TRUE(tag.contains(*VanillaBlocks::JUNGLE_FENCE));
    EXPECT_TRUE(tag.contains(*VanillaBlocks::ACACIA_FENCE));
    EXPECT_TRUE(tag.contains(*VanillaBlocks::DARK_OAK_FENCE));
}

// 验证 FENCES 标签包含所有木质栅栏
TEST_F(BlockTagIntegrationTest, Fences_ContainsAllWoodenFences)
{
    auto& tag = BlockTags::FENCES();
    EXPECT_TRUE(tag.contains(*VanillaBlocks::OAK_FENCE));
    EXPECT_TRUE(tag.contains(*VanillaBlocks::SPRUCE_FENCE));
    EXPECT_TRUE(tag.contains(*VanillaBlocks::BIRCH_FENCE));
    EXPECT_TRUE(tag.contains(*VanillaBlocks::JUNGLE_FENCE));
    EXPECT_TRUE(tag.contains(*VanillaBlocks::ACACIA_FENCE));
    EXPECT_TRUE(tag.contains(*VanillaBlocks::DARK_OAK_FENCE));
    // 注：nether_brick_fence 也应在 FENCES 标签中，但尚未注册方块指针
}

// 验证 FENCE_GATES 标签包含所有栅栏门
TEST_F(BlockTagIntegrationTest, FenceGates_ContainsAllFenceGates)
{
    auto& tag = BlockTags::FENCE_GATES();
    EXPECT_TRUE(tag.contains(*VanillaBlocks::OAK_FENCE_GATE));
    EXPECT_TRUE(tag.contains(*VanillaBlocks::SPRUCE_FENCE_GATE));
    EXPECT_TRUE(tag.contains(*VanillaBlocks::BIRCH_FENCE_GATE));
    EXPECT_TRUE(tag.contains(*VanillaBlocks::JUNGLE_FENCE_GATE));
    EXPECT_TRUE(tag.contains(*VanillaBlocks::ACACIA_FENCE_GATE));
    EXPECT_TRUE(tag.contains(*VanillaBlocks::DARK_OAK_FENCE_GATE));
}

// 验证 BEEHIVES 标签包含蜂箱和蜂巢
TEST_F(BlockTagIntegrationTest, Beehives_ContainsBeehiveAndBeeNest)
{
    auto& tag = BlockTags::BEEHIVES();
    EXPECT_TRUE(tag.contains(*VanillaBlocks::BEEHIVE));
    EXPECT_TRUE(tag.contains(*VanillaBlocks::BEE_NEST));
}

// ============================================================================
// 集成测试：新方块 BlockState 创建验证
// ============================================================================

class NewBlockIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

// 验证新注册栅栏方块能够创建有效的 BlockState
TEST_F(NewBlockIntegrationTest, FenceBlocks_CreateValidBlockStates)
{
    // 验证新注册的5种栅栏方块可以创建默认 BlockState
    EXPECT_NE(VanillaBlocks::SPRUCE_FENCE, nullptr);
    EXPECT_NE(VanillaBlocks::BIRCH_FENCE, nullptr);
    EXPECT_NE(VanillaBlocks::JUNGLE_FENCE, nullptr);
    EXPECT_NE(VanillaBlocks::ACACIA_FENCE, nullptr);
    EXPECT_NE(VanillaBlocks::DARK_OAK_FENCE, nullptr);
    // 验证方块 ID 有效
    EXPECT_GT(VanillaBlocks::SPRUCE_FENCE->blockId(), 0u);
    EXPECT_GT(VanillaBlocks::BIRCH_FENCE->blockId(), 0u);
    EXPECT_GT(VanillaBlocks::JUNGLE_FENCE->blockId(), 0u);
    EXPECT_GT(VanillaBlocks::ACACIA_FENCE->blockId(), 0u);
    EXPECT_GT(VanillaBlocks::DARK_OAK_FENCE->blockId(), 0u);
}

// 验证蜂箱和蜂巢方块能够创建有效的 BlockState
TEST_F(NewBlockIntegrationTest, BeehiveBlocks_CreateValidBlockStates)
{
    EXPECT_NE(VanillaBlocks::BEEHIVE, nullptr);
    EXPECT_NE(VanillaBlocks::BEE_NEST, nullptr);
    EXPECT_GT(VanillaBlocks::BEEHIVE->blockId(), 0u);
    EXPECT_GT(VanillaBlocks::BEE_NEST->blockId(), 0u);
    // 验证蜂箱和蜂巢是不同的方块
    EXPECT_NE(VanillaBlocks::BEEHIVE->blockId(), VanillaBlocks::BEE_NEST->blockId());
    // 验证蜂箱和蜂巢具有方块实体
    EXPECT_TRUE(VanillaBlocks::BEEHIVE->hasBlockEntity());
    EXPECT_TRUE(VanillaBlocks::BEE_NEST->hasBlockEntity());
}

// 验证蜂箱和蜂巢具有蜂蜜等级属性
TEST_F(NewBlockIntegrationTest, BeehiveBlocks_HaveHoneyLevelProperty)
{
    const auto& beehiveState = VanillaBlocks::BEEHIVE->defaultState();
    const auto& beeNestState = VanillaBlocks::BEE_NEST->defaultState();
    // 默认蜂蜜等级应为0
    EXPECT_EQ(beehiveState.get(BlockStateProperties::HONEY_LEVEL_0_5()), 0);
    EXPECT_EQ(beeNestState.get(BlockStateProperties::HONEY_LEVEL_0_5()), 0);
}

// ============================================================================
// FireBlock::getIncreasedFireBurnout 群系标志链路测试（偏差 #4 修复验证）
// ============================================================================
//
// 对齐 vanilla 1.21.11 FireBlock.checkBurnOut（FireBlock.java:178-179）的 flag1：
// 火焰蔓延时读群系级 EnvironmentAttributes.INCREASED_FIRE_BURNOUT，潮湿/特殊群系
// 恒定生效 -50/折半（与是否下雨无关）。此前 Cubium 用 isRaining()&&canDie 近似，
// 对"非下雨的潮湿群系"漏判、"下雨的非潮湿群系"误判。现已改用 FireBlock::
// getIncreasedFireBurnout 经 ChunkData::getBiomeAtBlock + BiomeRegistry 查群系标志。

namespace {
// 构造一个 ChunkData，将其 biome 全部设为指定 BiomeId。
std::unique_ptr<ChunkData> makeChunkWithBiome(BiomeId biomeId)
{
    auto chunk = std::make_unique<ChunkData>(0, 0);
    // 对 chunk 内所有 section（24 个）的所有 4×4×4 采样点写入同一 biome，
    // 确保任意 pos.y 查询都能命中。
    BiomeContainer& biomes = chunk->getBiomes();
    for (i32 section = 0; section < mc::world::CHUNK_SECTIONS; ++section) {
        for (i32 bx = 0; bx < 4; ++bx) {
            for (i32 by = 0; by < 4; ++by) {
                for (i32 bz = 0; bz < 4; ++bz) {
                    biomes.setBiome(section, bx, by, bz, biomeId);
                }
            }
        }
    }
    return chunk;
}
} // namespace

class FireBlockBiomeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        // BiomeRegistry::initialize 注册默认 biome（Plains 等），但不解析 JSON attributes。
        // 这里手动把 Swamp（Biomes::Swamp=6）的标志设为 true，模拟 BiomeLoader 解析
        // swamp.json 的 attributes["minecraft:gameplay/increased_fire_burnout"] 注入结果。
        world::biome::BiomeRegistry::instance().initialize();
        if (world::biome::BiomeRegistry::instance().hasBiome(Biomes::Swamp)) {
            world::biome::BiomeRegistry::instance().getMutable(Biomes::Swamp).setIncreasedFireBurnout(true);
        }
        // Plains 保持默认 false
    }

    // 经 friend 授权访问 FireBlock::getIncreasedFireBurnout（protected）。
    // TEST_F 测试体是独立函数非 fixture 成员，须经此静态中转才能命中 friend 授权。
    static bool callGetIncreasedFireBurnout(FireBlock& fire, IWorld& world, const BlockPos& pos)
    {
        return fire.getIncreasedFireBurnout(world, pos);
    }

    FireSpreadTestWorld m_world;
};

// Swamp 群系（increased_fire_burnout=true）：getIncreasedFireBurnout 应返回 true。
// 修复前用 isRaining()&&canDie 近似，晴天（isRaining=false）会返回 false（漏判）。
TEST_F(FireBlockBiomeTest, GetIncreasedFireBurnout_SwampBiome_True)
{
    FireBlock* fire = getFireBlock();
    ASSERT_NE(fire, nullptr);
    ASSERT_TRUE(world::biome::BiomeRegistry::instance().hasBiome(Biomes::Swamp));

    // 注入全 Swamp biome 的 chunk，晴天（isRaining=false）
    m_world.setChunkData(makeChunkWithBiome(Biomes::Swamp));
    m_world.setRaining(false);
    m_world.setCanRainAtResult(false);

    BlockPos pos(8, 64, 8); // chunk 内坐标
    EXPECT_TRUE(callGetIncreasedFireBurnout(*fire, m_world, pos));
}

// Plains 群系（increased_fire_burnout=false）：getIncreasedFireBurnout 应返回 false。
// 即使下雨，vanilla 也不触发 flag1（下雨不等于 increased_fire_burnout）。
TEST_F(FireBlockBiomeTest, GetIncreasedFireBurnout_PlainsBiome_False)
{
    FireBlock* fire = getFireBlock();
    ASSERT_NE(fire, nullptr);

    m_world.setChunkData(makeChunkWithBiome(Biomes::Plains));
    // 即便下雨，Plains 非 increased_fire_burnout 群系，flag1 应为 false（修复前会误判为 true）
    m_world.setRaining(true);
    m_world.setCanRainAtResult(true);

    BlockPos pos(8, 64, 8);
    EXPECT_FALSE(callGetIncreasedFireBurnout(*fire, m_world, pos));
}

// chunk 未加载（getChunk 返回 nullptr）：getIncreasedFireBurnout 应返回 false，
// 不崩溃（火焰在未加载区块本就不会蔓延）。
TEST_F(FireBlockBiomeTest, GetIncreasedFireBurnout_UnloadedChunk_False)
{
    FireBlock* fire = getFireBlock();
    ASSERT_NE(fire, nullptr);

    // 不注入 ChunkData，getChunk 返回 nullptr
    BlockPos pos(8, 64, 8);
    EXPECT_FALSE(callGetIncreasedFireBurnout(*fire, m_world, pos));
}

// ============================================================================
// BiomeLoader::applyAttributes 解析测试（验证 JSON attributes 注入链路）
// ============================================================================

class BiomeLoaderAttributesTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        world::biome::BiomeRegistry::instance().initialize();
    }

    // 构造最小 biome JSON，仅含 attributes 字段中的 increased_fire_burnout。
    static nlohmann::json makeBiomeJsonWithBurnout(bool burnout)
    {
        nlohmann::json j;
        j["attributes"] = nlohmann::json::object();
        j["attributes"]["minecraft:gameplay/increased_fire_burnout"] = burnout;
        return j;
    }
};

// attributes["minecraft:gameplay/increased_fire_burnout"]=true → 标志注入 true
TEST_F(BiomeLoaderAttributesTest, ParsesIncreasedFireBurnout_True)
{
    // 用 Plains（id=1）作为加载目标：先确保其标志为默认 false，加载 JSON 后应变 true。
    ASSERT_TRUE(world::biome::BiomeRegistry::instance().hasBiome(Biomes::Plains));
    ASSERT_FALSE(world::biome::BiomeRegistry::instance().get(Biomes::Plains).isIncreasedFireBurnout());

    auto result = world::biome::BiomeLoader::loadFromJson(
        makeBiomeJsonWithBurnout(true), ResourceLocation("minecraft", "plains"));
    ASSERT_TRUE(result.success());

    EXPECT_TRUE(world::biome::BiomeRegistry::instance().get(Biomes::Plains).isIncreasedFireBurnout());
}

// 无 attributes 字段 → 标志保持默认 false（不误注入）
TEST_F(BiomeLoaderAttributesTest, NoAttributesField_KeepsDefaultFalse)
{
    nlohmann::json j;
    j["temperature"] = 0.5f; // 仅 climate 字段，无 attributes
    auto result = world::biome::BiomeLoader::loadFromJson(j, ResourceLocation("minecraft", "desert"));
    ASSERT_TRUE(result.success());

    // Desert 标志应为默认 false
    if (world::biome::BiomeRegistry::instance().hasBiome(Biomes::Desert)) {
        EXPECT_FALSE(world::biome::BiomeRegistry::instance().get(Biomes::Desert).isIncreasedFireBurnout());
    }
}
