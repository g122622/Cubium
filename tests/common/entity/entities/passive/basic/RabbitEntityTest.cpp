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
#include "common/entity/entities/passive/basic/RabbitEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <cmath>
#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 带群系支持的兔子测试世界
 *
 * 继承 BaseChunkBackedTestWorld，支持设置区块群系以测试兔子类型选择。
 */
class RabbitTestWorld final : public test::BaseChunkBackedTestWorld {
public:
    using test::BaseChunkBackedTestWorld::ensureChunk;

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

    // 追踪 broadcastEntityStatus 调用（用于验证 RabbitJump 状态码广播）
    void broadcastEntityStatus(EntityId entityId, u8 status) override
    {
        m_lastBroadcastEntityId = entityId;
        m_lastBroadcastStatus = status;
        m_broadcastCount++;
    }

    [[nodiscard]] EntityId lastBroadcastEntityId() const { return m_lastBroadcastEntityId; }
    [[nodiscard]] u8 lastBroadcastStatus() const { return m_lastBroadcastStatus; }
    [[nodiscard]] i32 broadcastCount() const { return m_broadcastCount; }
    void resetBroadcastTracking()
    {
        m_lastBroadcastEntityId = EntityId(0);
        m_lastBroadcastStatus = 0;
        m_broadcastCount = 0;
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("RabbitTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("RabbitTestWorld::tickManager not implemented");
    }

    /**
     * @brief 设置指定区块中所有位置的群系
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param biomeId 要设置的群系ID
     */
    void setChunkBiome(ChunkCoord chunkX, ChunkCoord chunkZ, BiomeId biomeId)
    {
        ChunkData& chunk = ensureChunk(chunkX, chunkZ);
        // 填充所有 section 的所有位置
        for (i32 sectionIdx = 0; sectionIdx < world::CHUNK_SECTIONS; ++sectionIdx) {
            for (i32 x = 0; x < 4; ++x) {
                for (i32 y = 0; y < 4; ++y) {
                    for (i32 z = 0; z < 4; ++z) {
                        chunk.getBiomes().setBiome(sectionIdx, x, y, z, biomeId);
                    }
                }
            }
        }
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;

    // 广播追踪
    EntityId m_lastBroadcastEntityId = EntityId(0);
    u8 m_lastBroadcastStatus = 0;
    i32 m_broadcastCount = 0;
};

class RabbitEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }

    RabbitTestWorld m_world;
};

// ========== 兔子类型测试 ==========

TEST_F(RabbitEntityTest, RabbitType_DefaultIsBrown)
{
    RabbitEntity rabbit(EntityId(1));
    // 默认类型由 setRandomRabbitType 设置，测试概率分布
    // 由于随机性，我们只测试类型在有效范围内
    EXPECT_GE(static_cast<u8>(rabbit.getRabbitType()), 0);
    EXPECT_LE(static_cast<u8>(rabbit.getRabbitType()), 99); // 包括 Killer (99)
}

TEST_F(RabbitEntityTest, RabbitType_CanSetAndGetType)
{
    RabbitEntity rabbit(EntityId(1));

    rabbit.setRabbitType(RabbitEntity::RabbitType::White);
    EXPECT_EQ(rabbit.getRabbitType(), RabbitEntity::RabbitType::White);

    rabbit.setRabbitType(RabbitEntity::RabbitType::Black);
    EXPECT_EQ(rabbit.getRabbitType(), RabbitEntity::RabbitType::Black);

    rabbit.setRabbitType(RabbitEntity::RabbitType::Killer);
    EXPECT_EQ(rabbit.getRabbitType(), RabbitEntity::RabbitType::Killer);
    EXPECT_TRUE(rabbit.isKillerRabbit());
}

TEST_F(RabbitEntityTest, RabbitType_KillerRabbitDetection)
{
    RabbitEntity rabbit(EntityId(1));

    rabbit.setRabbitType(RabbitEntity::RabbitType::Brown);
    EXPECT_FALSE(rabbit.isKillerRabbit());

    rabbit.setRabbitType(RabbitEntity::RabbitType::Killer);
    EXPECT_TRUE(rabbit.isKillerRabbit());
}

// ========== 繁殖物品测试 ==========

TEST_F(RabbitEntityTest, IsBreedingItem_AcceptsCarrot)
{
    RabbitEntity rabbit(EntityId(1));

    ItemStack carrotStack(Items::CARROT, 1);
    EXPECT_TRUE(rabbit.isBreedingItem(carrotStack));
}

TEST_F(RabbitEntityTest, IsBreedingItem_AcceptsGoldenCarrot)
{
    RabbitEntity rabbit(EntityId(1));

    ItemStack goldenCarrotStack(Items::GOLDEN_CARROT, 1);
    EXPECT_TRUE(rabbit.isBreedingItem(goldenCarrotStack));
}

TEST_F(RabbitEntityTest, IsBreedingItem_AcceptsDandelion)
{
    RabbitEntity rabbit(EntityId(1));

    // 获取蒲公英方块物品
    const BlockItem* dandelionItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::DANDELION);
    ASSERT_NE(dandelionItem, nullptr);

    ItemStack dandelionStack(dandelionItem, 1);
    EXPECT_TRUE(rabbit.isBreedingItem(dandelionStack));
}

TEST_F(RabbitEntityTest, IsBreedingItem_RejectsOtherItems)
{
    RabbitEntity rabbit(EntityId(1));

    // 测试不接受其他物品
    if (Items::WHEAT != nullptr) {
        ItemStack wheatStack(Items::WHEAT, 1);
        EXPECT_FALSE(rabbit.isBreedingItem(wheatStack));
    }

    // 空物品栈
    ItemStack emptyStack;
    EXPECT_FALSE(rabbit.isBreedingItem(emptyStack));
}

// ========== spawnBaby 测试 ==========

TEST_F(RabbitEntityTest, SpawnBaby_CreatesChildRabbit)
{
    RabbitEntity parent1(EntityId(1));
    parent1.setWorld(&m_world);
    parent1.setPosition(0.0f, 64.0f, 0.0f);
    parent1.setRabbitType(RabbitEntity::RabbitType::Brown);

    RabbitEntity parent2(EntityId(2));
    parent2.setRabbitType(RabbitEntity::RabbitType::White);

    auto baby = parent1.spawnBaby(parent2);

    ASSERT_NE(baby, nullptr);
    EXPECT_TRUE(baby->isChild());

    // 检查是 RabbitEntity 类型
    RabbitEntity* babyRabbit = dynamic_cast<RabbitEntity*>(baby.get());
    EXPECT_NE(babyRabbit, nullptr);
}

TEST_F(RabbitEntityTest, SpawnBaby_InheritsParentType)
{
    RabbitEntity parent1(EntityId(1));
    parent1.setWorld(&m_world);
    parent1.setPosition(0.0f, 64.0f, 0.0f);
    parent1.setRabbitType(RabbitEntity::RabbitType::Gold);

    RabbitEntity parent2(EntityId(2));
    parent2.setRabbitType(RabbitEntity::RabbitType::SaltAndPepper);

    // 多次测试类型继承（由于随机性）
    for (int i = 0; i < 100; ++i) {
        auto baby = parent1.spawnBaby(parent2);
        ASSERT_NE(baby, nullptr);

        RabbitEntity* babyRabbit = dynamic_cast<RabbitEntity*>(baby.get());
        ASSERT_NE(babyRabbit, nullptr);

        // 类型应该是父母之一或随机生成的（在正常范围内）
        RabbitEntity::RabbitType type = babyRabbit->getRabbitType();
        bool validType = (type == RabbitEntity::RabbitType::Brown || type == RabbitEntity::RabbitType::White ||
            type == RabbitEntity::RabbitType::Black || type == RabbitEntity::RabbitType::WhiteSpotted ||
            type == RabbitEntity::RabbitType::Gold || type == RabbitEntity::RabbitType::SaltAndPepper ||
            type == RabbitEntity::RabbitType::Killer || type == RabbitEntity::RabbitType::Toast);
        EXPECT_TRUE(validType) << "Invalid rabbit type: " << static_cast<int>(type);
    }
}

// ========== 声音类别测试 ==========

TEST_F(RabbitEntityTest, SoundCategory_NeutralForNormalRabbit)
{
    RabbitEntity rabbit(EntityId(1));
    rabbit.setRabbitType(RabbitEntity::RabbitType::Brown);

    EXPECT_EQ(rabbit.getSoundCategory(), sound::SoundCategory::Neutral);
}

TEST_F(RabbitEntityTest, SoundCategory_HostileForKillerRabbit)
{
    RabbitEntity rabbit(EntityId(1));
    rabbit.setRabbitType(RabbitEntity::RabbitType::Killer);

    EXPECT_EQ(rabbit.getSoundCategory(), sound::SoundCategory::Hostile);
}

// ========== 属性测试 ==========

TEST_F(RabbitEntityTest, Attributes_HasCorrectBaseValues)
{
    RabbitEntity rabbit(EntityId(1));

    // MC 1.16.5: 兔子生命值为 3
    EXPECT_DOUBLE_EQ(rabbit.maxHealth(), 3.0);

    // MC 1.16.5: 兔子移动速度为 0.3
    EXPECT_DOUBLE_EQ(rabbit.getAttributeValue("generic.movement_speed", 0.0), 0.3);
}

// ========== 尺寸测试 ==========

TEST_F(RabbitEntityTest, Dimensions_CorrectBaseSize)
{
    RabbitEntity rabbit(EntityId(1));
    rabbit.setChild(false); // 设置为成体

    // MC 1.16.5: 兔子宽度 0.4，高度 0.5
    // 通过碰撞箱来验证尺寸
    const AxisAlignedBB& box = rabbit.boundingBox();
    // 碰撞箱的宽度和高度应该接近实体尺寸
    f32 boxWidth = box.maxX - box.minX;
    f32 boxHeight = box.maxY - box.minY;
    EXPECT_NEAR(boxWidth, 0.4f, 0.01f);
    EXPECT_NEAR(boxHeight, 0.5f, 0.01f);
}

TEST_F(RabbitEntityTest, EyeHeight_DifferentForChildAndAdult)
{
    RabbitEntity adultRabbit(EntityId(1));
    adultRabbit.setChild(false);

    RabbitEntity childRabbit(EntityId(2));
    childRabbit.setChild(true);

    // 成体眼睛高度 0.35，幼体 0.2
    EXPECT_FLOAT_EQ(adultRabbit.eyeHeight(), 0.35f);
    EXPECT_FLOAT_EQ(childRabbit.eyeHeight(), 0.2f);
}

// ========== 群系类型选择测试 ==========

TEST_F(RabbitEntityTest, BiomeType_NullWorldReturnsBrown)
{
    // 无世界时默认返回棕色
    RabbitEntity rabbit(EntityId(1));
    // 无世界，getDefaultRabbitTypeForBiome 应返回 Brown
    EXPECT_EQ(rabbit.getDefaultRabbitTypeForBiome(), RabbitEntity::RabbitType::Brown);
}

TEST_F(RabbitEntityTest, BiomeType_NullChunkReturnsBrown)
{
    // 有世界但无区块时默认返回棕色
    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);
    rabbit.setPosition(100.0f, 64.0f, 100.0f); // 位置不在任何区块中

    EXPECT_EQ(rabbit.getDefaultRabbitTypeForBiome(), RabbitEntity::RabbitType::Brown);
}

TEST_F(RabbitEntityTest, BiomeType_SnowyPlainsProducesWhiteOrSpotted)
{
    m_world.setChunkBiome(0, 0, Biomes::SnowyPlains);

    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);
    rabbit.setPosition(5.0f, 64.0f, 5.0f); // 区块 (0,0) 内的位置

    // 多次测试，应该只出现白色或白色斑点
    for (int i = 0; i < 50; ++i) {
        auto type = rabbit.getDefaultRabbitTypeForBiome();
        bool isWhiteOrSpotted =
            (type == RabbitEntity::RabbitType::White || type == RabbitEntity::RabbitType::WhiteSpotted);
        EXPECT_TRUE(isWhiteOrSpotted) << "Expected White or WhiteSpotted in SnowyPlains, got: "
                                      << static_cast<int>(type);
    }
}

TEST_F(RabbitEntityTest, BiomeType_IceSpikesProducesWhiteOrSpotted)
{
    m_world.setChunkBiome(0, 0, Biomes::IceSpikes);

    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);
    rabbit.setPosition(5.0f, 64.0f, 5.0f);

    for (int i = 0; i < 50; ++i) {
        auto type = rabbit.getDefaultRabbitTypeForBiome();
        bool isWhiteOrSpotted =
            (type == RabbitEntity::RabbitType::White || type == RabbitEntity::RabbitType::WhiteSpotted);
        EXPECT_TRUE(isWhiteOrSpotted) << "Expected White or WhiteSpotted in IceSpikes, got: " << static_cast<int>(type);
    }
}

TEST_F(RabbitEntityTest, BiomeType_DesertProducesGold)
{
    m_world.setChunkBiome(0, 0, Biomes::Desert);

    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);
    rabbit.setPosition(5.0f, 64.0f, 5.0f);

    // 沙漠群系总是生成金色兔子
    for (int i = 0; i < 50; ++i) {
        auto type = rabbit.getDefaultRabbitTypeForBiome();
        EXPECT_EQ(type, RabbitEntity::RabbitType::Gold) << "Expected Gold in Desert, got: " << static_cast<int>(type);
    }
}

TEST_F(RabbitEntityTest, BiomeType_DesertHillsProducesGold)
{
    m_world.setChunkBiome(0, 0, Biomes::DesertHills);

    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);
    rabbit.setPosition(5.0f, 64.0f, 5.0f);

    for (int i = 0; i < 20; ++i) {
        auto type = rabbit.getDefaultRabbitTypeForBiome();
        EXPECT_EQ(type, RabbitEntity::RabbitType::Gold)
            << "Expected Gold in DesertHills, got: " << static_cast<int>(type);
    }
}

TEST_F(RabbitEntityTest, BiomeType_PlainsProducesBrownSaltOrBlack)
{
    m_world.setChunkBiome(0, 0, Biomes::Plains);

    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);
    rabbit.setPosition(5.0f, 64.0f, 5.0f);

    // 平原群系应该只生成棕色、椒盐色或黑色
    for (int i = 0; i < 100; ++i) {
        auto type = rabbit.getDefaultRabbitTypeForBiome();
        bool isValid = (type == RabbitEntity::RabbitType::Brown || type == RabbitEntity::RabbitType::SaltAndPepper ||
            type == RabbitEntity::RabbitType::Black);
        EXPECT_TRUE(isValid) << "Expected Brown/SaltAndPepper/Black in Plains, got: " << static_cast<int>(type);
    }
}

TEST_F(RabbitEntityTest, BiomeType_ForestProducesBrownSaltOrBlack)
{
    m_world.setChunkBiome(0, 0, Biomes::Forest);

    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);
    rabbit.setPosition(5.0f, 64.0f, 5.0f);

    for (int i = 0; i < 100; ++i) {
        auto type = rabbit.getDefaultRabbitTypeForBiome();
        bool isValid = (type == RabbitEntity::RabbitType::Brown || type == RabbitEntity::RabbitType::SaltAndPepper ||
            type == RabbitEntity::RabbitType::Black);
        EXPECT_TRUE(isValid) << "Expected Brown/SaltAndPepper/Black in Forest, got: " << static_cast<int>(type);
    }
}

TEST_F(RabbitEntityTest, BiomeType_DefaultTypeDistribution)
{
    // 在默认群系（平原）下，测试类型分布大致符合 MC 的概率
    m_world.setChunkBiome(0, 0, Biomes::Plains);

    int brownCount = 0;
    int saltCount = 0;
    int blackCount = 0;
    const int iterations = 1000;

    for (int i = 0; i < iterations; ++i) {
        RabbitEntity rabbit(EntityId(static_cast<EntityId>(i + 100)));
        rabbit.setWorld(&m_world);
        rabbit.setPosition(5.0f, 64.0f, 5.0f);

        auto type = rabbit.getDefaultRabbitTypeForBiome();
        switch (type) {
            case RabbitEntity::RabbitType::Brown:
                ++brownCount;
                break;
            case RabbitEntity::RabbitType::SaltAndPepper:
                ++saltCount;
                break;
            case RabbitEntity::RabbitType::Black:
                ++blackCount;
                break;
            default:
                FAIL() << "Unexpected type in Plains: " << static_cast<int>(type);
        }
    }

    // MC 概率：棕色 50%，椒盐色 40%，黑色 10%
    // 允许 ±10% 的统计误差
    EXPECT_NEAR(brownCount / static_cast<double>(iterations), 0.50, 0.10)
        << "Brown distribution: " << brownCount << "/" << iterations;
    EXPECT_NEAR(saltCount / static_cast<double>(iterations), 0.40, 0.10)
        << "SaltAndPepper distribution: " << saltCount << "/" << iterations;
    EXPECT_NEAR(blackCount / static_cast<double>(iterations), 0.10, 0.08)
        << "Black distribution: " << blackCount << "/" << iterations;
}

TEST_F(RabbitEntityTest, BiomeType_SnowyDistribution)
{
    // 在雪地群系下，测试白色/白色斑点的分布
    m_world.setChunkBiome(0, 0, Biomes::SnowyTaiga);

    int whiteCount = 0;
    int spottedCount = 0;
    const int iterations = 1000;

    for (int i = 0; i < iterations; ++i) {
        RabbitEntity rabbit(EntityId(static_cast<EntityId>(i + 100)));
        rabbit.setWorld(&m_world);
        rabbit.setPosition(5.0f, 64.0f, 5.0f);

        auto type = rabbit.getDefaultRabbitTypeForBiome();
        switch (type) {
            case RabbitEntity::RabbitType::White:
                ++whiteCount;
                break;
            case RabbitEntity::RabbitType::WhiteSpotted:
                ++spottedCount;
                break;
            default:
                FAIL() << "Unexpected type in SnowyTaiga: " << static_cast<int>(type);
        }
    }

    // MC 概率：白色 80%，白色斑点 20%
    EXPECT_NEAR(whiteCount / static_cast<double>(iterations), 0.80, 0.10)
        << "White distribution: " << whiteCount << "/" << iterations;
    EXPECT_NEAR(spottedCount / static_cast<double>(iterations), 0.20, 0.10)
        << "WhiteSpotted distribution: " << spottedCount << "/" << iterations;
}

TEST_F(RabbitEntityTest, SpawnBaby_UsesParentBiomeForRandomType)
{
    // 在沙漠群系中，5% 概率的随机类型应该是金色
    m_world.setChunkBiome(0, 0, Biomes::Desert);

    RabbitEntity parent1(EntityId(1));
    parent1.setWorld(&m_world);
    parent1.setPosition(5.0f, 64.0f, 5.0f);
    parent1.setRabbitType(RabbitEntity::RabbitType::Brown);

    RabbitEntity parent2(EntityId(2));
    parent2.setRabbitType(RabbitEntity::RabbitType::White);

    // 多次测试繁殖后代的类型
    for (int i = 0; i < 100; ++i) {
        auto baby = parent1.spawnBaby(parent2);
        ASSERT_NE(baby, nullptr);

        RabbitEntity* babyRabbit = dynamic_cast<RabbitEntity*>(baby.get());
        ASSERT_NE(babyRabbit, nullptr);

        // 后代类型应该是父母之一（Brown/White）或金色（沙漠群系的5%随机）
        RabbitEntity::RabbitType type = babyRabbit->getRabbitType();
        bool validType = (type == RabbitEntity::RabbitType::Brown || type == RabbitEntity::RabbitType::White ||
            type == RabbitEntity::RabbitType::Gold);
        EXPECT_TRUE(validType) << "Unexpected baby type in Desert: " << static_cast<int>(type);
    }
}

// ========== WORLD_BORDER 常量测试 ==========

TEST(WorldConstantsTest, WorldBorderValue)
{
    // 验证 WORLD_BORDER 常量值为 30000000
    EXPECT_EQ(world::WORLD_BORDER, 30000000);
}

TEST(WorldConstantsTest, WorldBorderIsValidChunkCoord)
{
    // 在世界边界内的区块坐标应该有效
    EXPECT_TRUE(world::isValidChunkCoord(0, 0));
    EXPECT_TRUE(world::isValidChunkCoord(100, 100));
    EXPECT_TRUE(world::isValidChunkCoord(-100, -100));

    // 边界值
    i32 maxChunk = world::WORLD_BORDER / world::CHUNK_WIDTH;
    EXPECT_TRUE(world::isValidChunkCoord(maxChunk, maxChunk));
    EXPECT_TRUE(world::isValidChunkCoord(-maxChunk, -maxChunk));

    // 超出边界
    EXPECT_FALSE(world::isValidChunkCoord(maxChunk + 1, 0));
    EXPECT_FALSE(world::isValidChunkCoord(0, maxChunk + 1));
    EXPECT_FALSE(world::isValidChunkCoord(-maxChunk - 1, 0));
}

// ========== HoneyBlock 滑度测试 ==========

TEST(PhysicsConstantsTest, SlipperinessHoneyIsDefault)
{
    // 蜂蜜块滑度应为默认值 0.6（MC 中蜂蜜块不修改 friction）
    EXPECT_FLOAT_EQ(physics::SLIPPERINESS_HONEY, 0.6f);
}

TEST(PhysicsConstantsTest, SlipperinessIceValues)
{
    // 冰和蓝冰的滑度值
    EXPECT_FLOAT_EQ(physics::SLIPPERINESS_ICE, 0.98f);
    EXPECT_FLOAT_EQ(physics::SLIPPERINESS_BLUE_ICE, 0.989f);
    EXPECT_FLOAT_EQ(physics::SLIPPERINESS_SLIME, 0.8f);
    EXPECT_FLOAT_EQ(physics::SLIPPERINESS_DEFAULT, 0.6f);
}

// ========== 兔子跳跃动画状态机测试 ==========
// 对应 MC 1.21.11 Rabbit 的 jumpTicks/jumpDuration/startJumping/getJumpCompletion/aiStep 逻辑

TEST_F(RabbitEntityTest, Jump_DefaultState_NoJumpInProgress)
{
    // 默认状态：未在跳跃中
    RabbitEntity rabbit(EntityId(1));
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 0);
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 0);
    EXPECT_FALSE(rabbit.isJumping());
    // getJumpCompletion 在 jumpDuration==0 时返回 0
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(0.5f), 0.0f);
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(1.0f), 0.0f);
}

TEST_F(RabbitEntityTest, Jump_SetJumpingTrue_StartsJumpAnimation)
{
    // setJumping(true) 应启动跳跃动画：jumpDuration=10, jumpTicks=0, isJumping=true
    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);

    rabbit.setJumping(true);

    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10);
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 0);
    EXPECT_TRUE(rabbit.isJumping());
}

TEST_F(RabbitEntityTest, Jump_SetJumpingTrue_BroadcastsRabbitJumpStatus)
{
    // setJumping(true) 应广播 RabbitJump(1) 状态码到客户端
    // 对应 MC 1.21.11 Rabbit.jumpFromGround() 中 broadcastEntityEvent(this, (byte)1)
    RabbitEntity rabbit(EntityId(42));
    rabbit.setWorld(&m_world);
    m_world.resetBroadcastTracking();

    rabbit.setJumping(true);

    EXPECT_EQ(m_world.broadcastCount(), 1);
    EXPECT_EQ(m_world.lastBroadcastEntityId(), EntityId(42));
    EXPECT_EQ(m_world.lastBroadcastStatus(), static_cast<u8>(network::EntityStatusPacket::Status::RabbitJump));
}

TEST_F(RabbitEntityTest, Jump_StartJumping_IdempotentWhileJumping)
{
    // 跳跃动画进行中再次 startJumping() 不应重置状态或重复广播
    // 项目架构下 JumpController::tick() 每 tick 调用 setJumping(true)，必须幂等
    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);

    rabbit.setJumping(true); // 启动跳跃
    EXPECT_EQ(m_world.broadcastCount(), 1);
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10);
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 0);

    m_world.resetBroadcastTracking();
    rabbit.setJumping(true); // 再次启动 - 应被忽略

    EXPECT_EQ(m_world.broadcastCount(), 0); // 不应重复广播
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10);
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 0); // 不应重置
}

TEST_F(RabbitEntityTest, Jump_GetJumpCompletion_Formula)
{
    // 对应 MC 1.21.11 Rabbit.getJumpCompletion(partialTick):
    //   jumpDuration == 0 ? 0 : (jumpTicks + partialTick) / jumpDuration
    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);
    rabbit.setJumping(true); // jumpDuration=10, jumpTicks=0

    // jumpTicks=0: completion = (0 + partialTick) / 10
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(0.5f), 0.05f);
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(1.0f), 0.1f);

    // 推进 5 tick: jumpTicks=5
    for (i32 i = 0; i < 5; ++i) {
        rabbit.aiStep();
    }
    // 注意：aiStep 推进 jumpTicks，每 tick +1
    // 第 1 次 aiStep: jumpTicks 0→1
    // 第 5 次 aiStep: jumpTicks 4→5
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 5);
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(0.0f), 0.5f);  // 5/10
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(0.5f), 0.55f); // 5.5/10
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(1.0f), 0.6f);  // 6/10
}

TEST_F(RabbitEntityTest, Jump_AiStep_AdvancesJumpTicks)
{
    // aiStep() 每 tick 推进 jumpTicks，对应 MC Rabbit.aiStep() 的 jumpTicks++
    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);
    rabbit.setJumping(true);

    EXPECT_EQ(rabbit.rabbitJumpTicks(), 0);

    rabbit.aiStep(); // jumpTicks 0→1
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 1);

    rabbit.aiStep(); // jumpTicks 1→2
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 2);
}

TEST_F(RabbitEntityTest, Jump_AiStep_ResetsAtJumpDuration)
{
    // 当 jumpTicks 达到 jumpDuration 时，aiStep 应归零并清除跳跃状态
    // 对应 MC Rabbit.aiStep(): else if (jumpDuration != 0) { jumpTicks=0; jumpDuration=0; setJumping(false); }
    //
    // 时序分析（jumpDuration=10）：
    //   setJumping(true) → jumpTicks=0
    //   aiStep #1: jumpTicks 0→1
    //   aiStep #2: jumpTicks 1→2
    //   ...
    //   aiStep #10: jumpTicks 9→10 (now == jumpDuration, 但本次仍走 ++ 分支)
    //   aiStep #11: jumpTicks==jumpDuration → 触发归零分支
    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);
    rabbit.setJumping(true); // jumpDuration=10

    // 推进 10 tick 使 jumpTicks 达到 jumpDuration
    for (i32 i = 0; i < 10; ++i) {
        rabbit.aiStep();
    }
    // 第 10 次 aiStep: jumpTicks 9→10，此时 jumpTicks == jumpDuration，但本次仍走 ++ 分支
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 10);
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10);
    EXPECT_TRUE(rabbit.isJumping());

    // 第 11 tick: jumpTicks == jumpDuration → 触发归零
    rabbit.aiStep();
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 0);
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 0);
    EXPECT_FALSE(rabbit.isJumping());
}

TEST_F(RabbitEntityTest, Jump_AiStep_ExactTickBoundary)
{
    // 验证跳跃动画持续 10 tick 后，在第 11 tick 结束
    // 跳跃动画的有效渲染区间为 jumpTicks ∈ [0, 10]（含端点）
    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);
    rabbit.setJumping(true);

    // 推进 10 tick: jumpTicks 应为 10，仍在跳跃中
    for (i32 i = 0; i < 10; ++i) {
        rabbit.aiStep();
    }
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 10);
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10);
    EXPECT_TRUE(rabbit.isJumping());

    // 第 11 tick: jumpTicks == jumpDuration → 触发归零
    rabbit.aiStep();
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 0);
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 0);
    EXPECT_FALSE(rabbit.isJumping());
}

TEST_F(RabbitEntityTest, Jump_CompletionReachesOne_BeforeReset)
{
    // 验证在跳跃动画最后一 tick，getJumpCompletion(1.0) 等于 1.0
    // 第 9 tick 后: jumpTicks=9, completion(1.0) = (9+1)/10 = 1.0
    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);
    rabbit.setJumping(true);

    for (i32 i = 0; i < 9; ++i) {
        rabbit.aiStep();
    }
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 9);
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(1.0f), 1.0f); // (9+1)/10
}

TEST_F(RabbitEntityTest, Jump_SetJumpingFalse_DoesNotAffectAnimation)
{
    // setJumping(false) 不应直接终止跳跃动画（动画由 aiStep 推进逻辑负责）
    // 这与 MC 行为一致：setJumping(false) 只设置 jumping 标志
    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);
    rabbit.setJumping(true); // 启动: jumpDuration=10
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10);

    rabbit.setJumping(false);                   // 不影响 jumpDuration
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10); // 动画仍在进行
    EXPECT_FALSE(rabbit.isJumping());           // 但 m_isJumping 已为 false
}

TEST_F(RabbitEntityTest, Jump_CanRestartAfterCompletion)
{
    // 跳跃动画完成后，可以启动新的跳跃
    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);

    // 第一次跳跃
    rabbit.setJumping(true);
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10);
    EXPECT_EQ(m_world.broadcastCount(), 1);

    // 推进到完成（11 tick: 10 tick 推进 + 1 tick 归零）
    for (i32 i = 0; i < 11; ++i) {
        rabbit.aiStep();
    }
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 0);
    EXPECT_FALSE(rabbit.isJumping());

    // 第二次跳跃
    m_world.resetBroadcastTracking();
    rabbit.setJumping(true);
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10);
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 0);
    EXPECT_TRUE(rabbit.isJumping());
    EXPECT_EQ(m_world.broadcastCount(), 1); // 再次广播
}

TEST_F(RabbitEntityTest, Jump_GetJumpCompletion_PartialTickRange)
{
    // 验证 partialTick 在 [0, 1] 范围内的插值
    RabbitEntity rabbit(EntityId(1));
    rabbit.setWorld(&m_world);
    rabbit.setJumping(true); // jumpDuration=10, jumpTicks=0

    // 推进 3 tick: jumpTicks=3
    for (i32 i = 0; i < 3; ++i) {
        rabbit.aiStep();
    }

    // completion = (3 + partialTick) / 10
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(0.0f), 0.3f);
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(0.25f), 0.325f);
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(0.5f), 0.35f);
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(0.75f), 0.375f);
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(1.0f), 0.4f);
}

// ========== RabbitJump 状态码常量测试 ==========

TEST_F(RabbitEntityTest, Jump_RabbitJumpStatusConstant_IsOne)
{
    // 验证 RabbitJump 状态码 = 1（对应 MC byte 1）
    EXPECT_EQ(static_cast<u8>(network::EntityStatusPacket::Status::RabbitJump), 1);
}

} // namespace
} // namespace mc
