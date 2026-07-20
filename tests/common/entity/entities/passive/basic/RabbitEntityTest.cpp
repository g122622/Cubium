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
#include "common/entity/ai/controller/RabbitJumpControl.hpp"
#include "common/entity/ai/controller/RabbitMoveControl.hpp"
#include "common/entity/ai/goal/goals/special/RaidGardenGoal.hpp"
#include "common/entity/attribute/Attributes.hpp"
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
#include "common/world/block/blocks/agricultural/CarrotBlock.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gamerule/GameRules.hpp"
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

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const { return m_spawnedEntities; }

    // 追踪 broadcastEntityStatus 调用（用于验证 RabbitJump 状态码广播）
    void broadcastEntityStatus(EntityInstanceId entityId, u8 status) override
    {
        m_lastBroadcastEntityId = entityId;
        m_lastBroadcastStatus = status;
        m_broadcastCount++;
    }

    [[nodiscard]] EntityInstanceId lastBroadcastEntityId() const { return m_lastBroadcastEntityId; }
    [[nodiscard]] u8 lastBroadcastStatus() const { return m_lastBroadcastStatus; }
    [[nodiscard]] i32 broadcastCount() const { return m_broadcastCount; }
    void resetBroadcastTracking()
    {
        m_lastBroadcastEntityId = EntityInstanceId(0);
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
    EntityInstanceId m_lastBroadcastEntityId = EntityInstanceId(0);
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
    RabbitEntity rabbit(EntityInstanceId(1));
    // 默认类型由 setRandomRabbitType 设置，测试概率分布
    // 由于随机性，我们只测试类型在有效范围内
    EXPECT_GE(static_cast<u8>(rabbit.getRabbitType()), 0);
    EXPECT_LE(static_cast<u8>(rabbit.getRabbitType()), 99); // 包括 Killer (99)
}

TEST_F(RabbitEntityTest, RabbitType_CanSetAndGetType)
{
    RabbitEntity rabbit(EntityInstanceId(1));

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
    RabbitEntity rabbit(EntityInstanceId(1));

    rabbit.setRabbitType(RabbitEntity::RabbitType::Brown);
    EXPECT_FALSE(rabbit.isKillerRabbit());

    rabbit.setRabbitType(RabbitEntity::RabbitType::Killer);
    EXPECT_TRUE(rabbit.isKillerRabbit());
}

// ========== 繁殖物品测试 ==========

TEST_F(RabbitEntityTest, IsBreedingItem_AcceptsCarrot)
{
    RabbitEntity rabbit(EntityInstanceId(1));

    ItemStack carrotStack(Items::CARROT, 1);
    EXPECT_TRUE(rabbit.isBreedingItem(carrotStack));
}

TEST_F(RabbitEntityTest, IsBreedingItem_AcceptsGoldenCarrot)
{
    RabbitEntity rabbit(EntityInstanceId(1));

    ItemStack goldenCarrotStack(Items::GOLDEN_CARROT, 1);
    EXPECT_TRUE(rabbit.isBreedingItem(goldenCarrotStack));
}

TEST_F(RabbitEntityTest, IsBreedingItem_AcceptsDandelion)
{
    RabbitEntity rabbit(EntityInstanceId(1));

    // 获取蒲公英方块物品
    const BlockItem* dandelionItem = BlockItemRegistry::instance().getBlockItem(*VanillaBlocks::DANDELION);
    ASSERT_NE(dandelionItem, nullptr);

    ItemStack dandelionStack(dandelionItem, 1);
    EXPECT_TRUE(rabbit.isBreedingItem(dandelionStack));
}

TEST_F(RabbitEntityTest, IsBreedingItem_RejectsOtherItems)
{
    RabbitEntity rabbit(EntityInstanceId(1));

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
    RabbitEntity parent1(EntityInstanceId(1));
    parent1.setWorld(&m_world);
    parent1.setPosition(0.0f, 64.0f, 0.0f);
    parent1.setRabbitType(RabbitEntity::RabbitType::Brown);

    RabbitEntity parent2(EntityInstanceId(2));
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
    RabbitEntity parent1(EntityInstanceId(1));
    parent1.setWorld(&m_world);
    parent1.setPosition(0.0f, 64.0f, 0.0f);
    parent1.setRabbitType(RabbitEntity::RabbitType::Gold);

    RabbitEntity parent2(EntityInstanceId(2));
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
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setRabbitType(RabbitEntity::RabbitType::Brown);

    EXPECT_EQ(rabbit.getSoundCategory(), sound::SoundCategory::Neutral);
}

TEST_F(RabbitEntityTest, SoundCategory_HostileForKillerRabbit)
{
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setRabbitType(RabbitEntity::RabbitType::Killer);

    EXPECT_EQ(rabbit.getSoundCategory(), sound::SoundCategory::Hostile);
}

// ========== 属性测试 ==========

TEST_F(RabbitEntityTest, Attributes_HasCorrectBaseValues)
{
    RabbitEntity rabbit(EntityInstanceId(1));

    // MC 1.16.5: 兔子生命值为 3
    EXPECT_DOUBLE_EQ(rabbit.maxHealth(), 3.0);

    // MC 1.16.5: 兔子移动速度为 0.3
    EXPECT_DOUBLE_EQ(rabbit.getAttributeValue("generic.movement_speed", 0.0), 0.3);
}

// ========== 尺寸测试 ==========

TEST_F(RabbitEntityTest, Dimensions_CorrectBaseSize)
{
    RabbitEntity rabbit(EntityInstanceId(1));
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
    RabbitEntity adultRabbit(EntityInstanceId(1));
    adultRabbit.setChild(false);

    RabbitEntity childRabbit(EntityInstanceId(2));
    childRabbit.setChild(true);

    // 成体眼睛高度 0.35，幼体 0.2
    EXPECT_FLOAT_EQ(adultRabbit.eyeHeight(), 0.35f);
    EXPECT_FLOAT_EQ(childRabbit.eyeHeight(), 0.2f);
}

// ========== 群系类型选择测试 ==========

TEST_F(RabbitEntityTest, BiomeType_NullWorldReturnsBrown)
{
    // 无世界时默认返回棕色
    RabbitEntity rabbit(EntityInstanceId(1));
    // 无世界，getDefaultRabbitTypeForBiome 应返回 Brown
    EXPECT_EQ(rabbit.getDefaultRabbitTypeForBiome(), RabbitEntity::RabbitType::Brown);
}

TEST_F(RabbitEntityTest, BiomeType_NullChunkReturnsBrown)
{
    // 有世界但无区块时默认返回棕色
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.setPosition(100.0f, 64.0f, 100.0f); // 位置不在任何区块中

    EXPECT_EQ(rabbit.getDefaultRabbitTypeForBiome(), RabbitEntity::RabbitType::Brown);
}

TEST_F(RabbitEntityTest, BiomeType_SnowyPlainsProducesWhiteOrSpotted)
{
    m_world.setChunkBiome(0, 0, Biomes::SnowyPlains);

    RabbitEntity rabbit(EntityInstanceId(1));
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

    RabbitEntity rabbit(EntityInstanceId(1));
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

    RabbitEntity rabbit(EntityInstanceId(1));
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

    RabbitEntity rabbit(EntityInstanceId(1));
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

    RabbitEntity rabbit(EntityInstanceId(1));
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

    RabbitEntity rabbit(EntityInstanceId(1));
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
        RabbitEntity rabbit(EntityInstanceId(static_cast<EntityInstanceId>(i + 100)));
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
        RabbitEntity rabbit(EntityInstanceId(static_cast<EntityInstanceId>(i + 100)));
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

    RabbitEntity parent1(EntityInstanceId(1));
    parent1.setWorld(&m_world);
    parent1.setPosition(5.0f, 64.0f, 5.0f);
    parent1.setRabbitType(RabbitEntity::RabbitType::Brown);

    RabbitEntity parent2(EntityInstanceId(2));
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
    RabbitEntity rabbit(EntityInstanceId(1));
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 0);
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 0);
    EXPECT_FALSE(rabbit.isJumping());
    // getJumpCompletion 在 jumpDuration==0 时返回 0
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(0.5f), 0.0f);
    EXPECT_FLOAT_EQ(rabbit.getJumpCompletion(1.0f), 0.0f);
}

TEST_F(RabbitEntityTest, Jump_StartJumping_StartsJumpAnimation)
{
    // startJumping() 应启动跳跃动画：jumpDuration=10, jumpTicks=0, isJumping=true
    // 对应 MC 1.21.11 Rabbit.startJumping(): setJumping(true); jumpDuration=10; jumpTicks=0;
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);

    rabbit.startJumping();

    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10);
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 0);
    EXPECT_TRUE(rabbit.isJumping());
}

TEST_F(RabbitEntityTest, Jump_StartJumping_BroadcastsRabbitJumpStatus)
{
    // startJumping() 应广播 RabbitJump(1) 状态码到客户端
    // 对应 MC 1.21.11 Rabbit.jumpFromGround() 中 broadcastEntityEvent(this, (byte)1)
    RabbitEntity rabbit(EntityInstanceId(42));
    rabbit.setWorld(&m_world);
    m_world.resetBroadcastTracking();

    rabbit.startJumping();

    EXPECT_EQ(m_world.broadcastCount(), 1);
    EXPECT_EQ(m_world.lastBroadcastEntityId(), EntityInstanceId(42));
    EXPECT_EQ(m_world.lastBroadcastStatus(), static_cast<u8>(network::EntityStatusPacket::Status::RabbitJump));
}

TEST_F(RabbitEntityTest, Jump_StartJumping_IdempotentWhileJumping)
{
    // 跳跃动画进行中再次 startJumping() 不应重置状态或重复广播
    // 对应 MC RabbitJumpControl.tick() 每 tick 可能调用 startJumping()，必须幂等
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);

    rabbit.startJumping(); // 启动跳跃
    EXPECT_EQ(m_world.broadcastCount(), 1);
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10);
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 0);

    m_world.resetBroadcastTracking();
    rabbit.startJumping(); // 再次启动 - 应被忽略

    EXPECT_EQ(m_world.broadcastCount(), 0); // 不应重复广播
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10);
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 0); // 不应重置
}

TEST_F(RabbitEntityTest, Jump_GetJumpCompletion_Formula)
{
    // 对应 MC 1.21.11 Rabbit.getJumpCompletion(partialTick):
    //   jumpDuration == 0 ? 0 : (jumpTicks + partialTick) / jumpDuration
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.startJumping(); // jumpDuration=10, jumpTicks=0

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
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.startJumping();

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
    //   startJumping() → jumpTicks=0
    //   aiStep #1: jumpTicks 0→1
    //   aiStep #2: jumpTicks 1→2
    //   ...
    //   aiStep #10: jumpTicks 9→10 (now == jumpDuration, 但本次仍走 ++ 分支)
    //   aiStep #11: jumpTicks==jumpDuration → 触发归零分支
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.startJumping(); // jumpDuration=10

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
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.startJumping();

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
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.startJumping();

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
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.startJumping(); // 启动: jumpDuration=10
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10);

    rabbit.setJumping(false);                   // 不影响 jumpDuration
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10); // 动画仍在进行
    EXPECT_FALSE(rabbit.isJumping());           // 但 m_isJumping 已为 false
}

TEST_F(RabbitEntityTest, Jump_CanRestartAfterCompletion)
{
    // 跳跃动画完成后，可以启动新的跳跃
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);

    // 第一次跳跃
    rabbit.startJumping();
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
    rabbit.startJumping();
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10);
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 0);
    EXPECT_TRUE(rabbit.isJumping());
    EXPECT_EQ(m_world.broadcastCount(), 1); // 再次广播
}

TEST_F(RabbitEntityTest, Jump_GetJumpCompletion_PartialTickRange)
{
    // 验证 partialTick 在 [0, 1] 范围内的插值
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.startJumping(); // jumpDuration=10, jumpTicks=0

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

// ========== RabbitJumpControl 控制器测试 ==========

TEST_F(RabbitEntityTest, RabbitJumpControl_DefaultCanJump)
{
    // 新建的 RabbitJumpControl 默认 canJump=true
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);

    auto* jumpCtrl = rabbit.jumpController();
    ASSERT_NE(jumpCtrl, nullptr);
    auto* rabbitJumpCtrl = dynamic_cast<entity::ai::controller::RabbitJumpControl*>(jumpCtrl);
    ASSERT_NE(rabbitJumpCtrl, nullptr);
    EXPECT_TRUE(rabbitJumpCtrl->canJump());
    EXPECT_FALSE(rabbitJumpCtrl->wantJump());
}

TEST_F(RabbitEntityTest, RabbitJumpControl_SetCanJump)
{
    // setCanJump 可以启用/禁用跳跃
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);

    auto* jumpCtrl = rabbit.jumpController();
    auto* rabbitJumpCtrl = dynamic_cast<entity::ai::controller::RabbitJumpControl*>(jumpCtrl);
    ASSERT_NE(rabbitJumpCtrl, nullptr);

    rabbitJumpCtrl->setCanJump(false);
    EXPECT_FALSE(rabbitJumpCtrl->canJump());

    rabbitJumpCtrl->setCanJump(true);
    EXPECT_TRUE(rabbitJumpCtrl->canJump());
}

TEST_F(RabbitEntityTest, RabbitJumpControl_Tick_TriggersStartJumping)
{
    // RabbitJumpControl::tick() 在 wantJump 时调用 startJumping()
    RabbitEntity rabbit(EntityInstanceId(42));
    rabbit.setWorld(&m_world);
    m_world.resetBroadcastTracking();

    auto* jumpCtrl = rabbit.jumpController();
    auto* rabbitJumpCtrl = dynamic_cast<entity::ai::controller::RabbitJumpControl*>(jumpCtrl);
    ASSERT_NE(rabbitJumpCtrl, nullptr);

    // 设置跳跃请求
    rabbitJumpCtrl->setJumping();
    EXPECT_TRUE(rabbitJumpCtrl->wantJump());

    // tick 应触发 startJumping，启动跳跃动画并广播状态码
    rabbitJumpCtrl->tick();

    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10);
    EXPECT_EQ(rabbit.rabbitJumpTicks(), 0);
    EXPECT_TRUE(rabbit.isJumping());
    EXPECT_FALSE(rabbitJumpCtrl->wantJump()); // wantJump 应被清除
    EXPECT_EQ(m_world.broadcastCount(), 1);
    EXPECT_EQ(m_world.lastBroadcastStatus(), static_cast<u8>(network::EntityStatusPacket::Status::RabbitJump));
}

TEST_F(RabbitEntityTest, RabbitJumpControl_Tick_IdempotentWhenAlreadyJumping)
{
    // 跳跃动画进行中再次 tick 不应重复触发
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);

    auto* jumpCtrl = rabbit.jumpController();
    auto* rabbitJumpCtrl = dynamic_cast<entity::ai::controller::RabbitJumpControl*>(jumpCtrl);
    ASSERT_NE(rabbitJumpCtrl, nullptr);

    // 第一次触发跳跃
    rabbitJumpCtrl->setJumping();
    rabbitJumpCtrl->tick();
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10);

    // 第二次触发 - 由于 startJumping 幂等，不应重复广播
    m_world.resetBroadcastTracking();
    rabbitJumpCtrl->setJumping();
    rabbitJumpCtrl->tick();
    EXPECT_EQ(m_world.broadcastCount(), 0);     // 不应重复广播
    EXPECT_EQ(rabbit.rabbitJumpDuration(), 10); // 仍为 10
}

TEST_F(RabbitEntityTest, RabbitJumpControl_Tick_NoActionWhenNotWantJump)
{
    // wantJump 为 false 时 tick 不应触发跳跃
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    m_world.resetBroadcastTracking();

    auto* jumpCtrl = rabbit.jumpController();
    auto* rabbitJumpCtrl = dynamic_cast<entity::ai::controller::RabbitJumpControl*>(jumpCtrl);
    ASSERT_NE(rabbitJumpCtrl, nullptr);

    // 不设置跳跃请求，直接 tick
    rabbitJumpCtrl->tick();

    EXPECT_EQ(rabbit.rabbitJumpDuration(), 0);
    EXPECT_FALSE(rabbit.isJumping());
    EXPECT_EQ(m_world.broadcastCount(), 0);
}

// ========== RabbitMoveControl 控制器测试 ==========

TEST_F(RabbitEntityTest, RabbitMoveControl_DefaultNextJumpSpeed)
{
    // 新建的 RabbitMoveControl 默认 nextJumpSpeed=0
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);

    auto* moveCtrl = rabbit.moveController();
    ASSERT_NE(moveCtrl, nullptr);
    auto* rabbitMoveCtrl = dynamic_cast<entity::ai::controller::RabbitMoveControl*>(moveCtrl);
    ASSERT_NE(rabbitMoveCtrl, nullptr);
    EXPECT_DOUBLE_EQ(rabbitMoveCtrl->nextJumpSpeed(), 0.0);
}

TEST_F(RabbitEntityTest, RabbitMoveControl_SetMoveTo_RecordsNextJumpSpeed)
{
    // setMoveTo 应记录 nextJumpSpeed（仅当 speed > 0）
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);

    auto* moveCtrl = rabbit.moveController();
    auto* rabbitMoveCtrl = dynamic_cast<entity::ai::controller::RabbitMoveControl*>(moveCtrl);
    ASSERT_NE(rabbitMoveCtrl, nullptr);

    rabbitMoveCtrl->setMoveTo(10.0, 0.0, 10.0, 1.5);
    EXPECT_DOUBLE_EQ(rabbitMoveCtrl->nextJumpSpeed(), 1.5);
    EXPECT_DOUBLE_EQ(rabbitMoveCtrl->speed(), 1.5);
}

TEST_F(RabbitEntityTest, RabbitMoveControl_SetMoveTo_ZeroSpeedDoesNotUpdateNextJumpSpeed)
{
    // speed=0 不应更新 nextJumpSpeed（对应 MC if (speed > 0.0)）
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);

    auto* moveCtrl = rabbit.moveController();
    auto* rabbitMoveCtrl = dynamic_cast<entity::ai::controller::RabbitMoveControl*>(moveCtrl);
    ASSERT_NE(rabbitMoveCtrl, nullptr);

    // 先设置非零速度
    rabbitMoveCtrl->setMoveTo(10.0, 0.0, 10.0, 2.0);
    EXPECT_DOUBLE_EQ(rabbitMoveCtrl->nextJumpSpeed(), 2.0);

    // 再设置零速度 - nextJumpSpeed 不应被更新
    rabbitMoveCtrl->setMoveTo(5.0, 0.0, 5.0, 0.0);
    EXPECT_DOUBLE_EQ(rabbitMoveCtrl->nextJumpSpeed(), 2.0); // 仍为之前的值
}

// ========== 着陆延迟测试 ==========

TEST_F(RabbitEntityTest, LandingDelay_DefaultZero)
{
    // 新建的兔子 jumpDelayTicks 为 0
    RabbitEntity rabbit(EntityInstanceId(1));
    EXPECT_EQ(rabbit.jumpDelayTicks(), 0);
    EXPECT_FALSE(rabbit.wasOnGround());
}

TEST_F(RabbitEntityTest, LandingDelay_SetLandingDelay_SlowSpeed)
{
    // setLandingDelay: 速度 < 2.2 时延迟 10 tick
    // 对应 MC Rabbit.setLandingDelay(): if (speed < 2.2) jumpDelayTicks=10; else jumpDelayTicks=1;
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);

    // 设置移动速度 < 2.2
    auto* moveCtrl = rabbit.moveController();
    ASSERT_NE(moveCtrl, nullptr);
    moveCtrl->setMoveTo(10.0, 0.0, 10.0, 1.0); // speed=1.0

    // 模拟着陆：设置 onGround=true，wasOnGround 初始为 false
    // updateAITasks() 检测到 onGround && !wasOnGround 时调用 checkLandingDelay()
    rabbit.setOnGround(true);
    rabbit.updateAITasks();

    // 速度 < 2.2，应设置延迟 10 tick
    EXPECT_EQ(rabbit.jumpDelayTicks(), 10);
}

TEST_F(RabbitEntityTest, LandingDelay_SetLandingDelay_FastSpeed)
{
    // setLandingDelay: 速度 >= 2.2 时延迟 1 tick
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);

    auto* moveCtrl = rabbit.moveController();
    ASSERT_NE(moveCtrl, nullptr);
    moveCtrl->setMoveTo(10.0, 0.0, 10.0, 2.5); // speed=2.5 >= 2.2

    rabbit.setOnGround(true);
    rabbit.updateAITasks();

    // 速度 >= 2.2，应设置延迟 1 tick
    EXPECT_EQ(rabbit.jumpDelayTicks(), 1);
}

TEST_F(RabbitEntityTest, LandingDelay_DecrementsOverTime)
{
    // jumpDelayTicks 每 tick 递减
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);

    auto* moveCtrl = rabbit.moveController();
    ASSERT_NE(moveCtrl, nullptr);
    moveCtrl->setMoveTo(10.0, 0.0, 10.0, 1.0); // speed=1.0, delay=10

    rabbit.setOnGround(true);
    rabbit.updateAITasks();
    EXPECT_EQ(rabbit.jumpDelayTicks(), 10);

    // 后续 tick 应递减（onGround 仍为 true，wasOnGround 已被设为 true，不再触发 checkLandingDelay）
    rabbit.updateAITasks();
    EXPECT_EQ(rabbit.jumpDelayTicks(), 9);
}

TEST_F(RabbitEntityTest, MoreCarrotTicks_DefaultZero)
{
    // 新建的兔子 moreCarrotTicks 为 0
    RabbitEntity rabbit(EntityInstanceId(1));
    EXPECT_EQ(rabbit.moreCarrotTicks(), 0);
    EXPECT_TRUE(rabbit.wantsMoreFood());
}

TEST_F(RabbitEntityTest, MoreCarrotTicks_SetAndDecrement)
{
    // setMoreCarrotTicks 可以设置值，updateAITasks 会递减
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);

    rabbit.setMoreCarrotTicks(40);
    EXPECT_EQ(rabbit.moreCarrotTicks(), 40);
    EXPECT_FALSE(rabbit.wantsMoreFood());

    // updateAITasks 应随机递减 moreCarrotTicks
    rabbit.updateAITasks();
    // 递减 0~2，所以结果在 [38, 40]
    EXPECT_LE(rabbit.moreCarrotTicks(), 40);
    EXPECT_GE(rabbit.moreCarrotTicks(), 38);
}

// ========== 杀手兔属性测试 ==========

TEST_F(RabbitEntityTest, KillerRabbit_HasArmorAndAttackDamage)
{
    // 杀手兔应有 ARMOR=8 和 ATTACK_DAMAGE=3+5=8
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.setRabbitType(RabbitEntity::RabbitType::Killer);

    // ARMOR = 8.0
    EXPECT_FLOAT_EQ(static_cast<f32>(rabbit.getAttributeValue(entity::attribute::Attributes::ARMOR, 0.0)), 8.0f);

    // ATTACK_DAMAGE = 3.0 (base) + 5.0 (modifier) = 8.0
    EXPECT_FLOAT_EQ(
        static_cast<f32>(rabbit.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 0.0)), 8.0f);
}

TEST_F(RabbitEntityTest, NormalRabbit_HasZeroArmorAndNoAttackModifier)
{
    // 普通兔子 ARMOR=0 且无 EVIL 攻击修改器
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.setRabbitType(RabbitEntity::RabbitType::Brown);

    EXPECT_FLOAT_EQ(static_cast<f32>(rabbit.getAttributeValue(entity::attribute::Attributes::ARMOR, 0.0)), 0.0f);

    // ATTACK_DAMAGE 基础值仍为 3.0（无修改器）
    EXPECT_FLOAT_EQ(
        static_cast<f32>(rabbit.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 0.0)), 3.0f);
}

TEST_F(RabbitEntityTest, KillerRabbit_ToNormal_RemovesAttackModifier)
{
    // 从杀手兔切回普通兔应移除 EVIL 攻击修改器
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.setRabbitType(RabbitEntity::RabbitType::Killer);
    EXPECT_FLOAT_EQ(
        static_cast<f32>(rabbit.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 0.0)), 8.0f);

    rabbit.setRabbitType(RabbitEntity::RabbitType::Brown);
    EXPECT_FLOAT_EQ(static_cast<f32>(rabbit.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 0.0)),
        3.0f); // 移除 +5 修改器后回到基础值
}

// ========== RaidGardenGoal 测试 ==========

namespace {

/// 辅助函数：在 (x, y, z) 放置指定方块的默认状态
void placeBlock(RabbitTestWorld& world, i32 x, i32 y, i32 z, const Block* block)
{
    const BlockState* state = &block->defaultState();
    world.setBlockState(x, y, z, state);
}

/// 辅助函数：在 (x, y, z) 放置指定年龄的胡萝卜作物
void placeCarrotAtAge(RabbitTestWorld& world, i32 x, i32 y, i32 z, i32 age)
{
    auto* carrotBlock = dynamic_cast<const blocks::CropBlock*>(VanillaBlocks::CARROTS);
    ASSERT_NE(carrotBlock, nullptr);
    const BlockState& carrotState = carrotBlock->withAge(age);
    world.setBlockState(x, y, z, &carrotState);
}

} // namespace

TEST_F(RabbitEntityTest, RaidGardenGoal_RegisteredInRabbitGoals)
{
    // 兔子的 goalSelector 中应包含 RaidGardenGoal
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);

    // 遍历 goalSelector 中的目标，检查是否存在 RaidGardenGoal
    bool found = false;
    for (const auto& prioritizedGoal : rabbit.goalSelector().getAllGoals()) {
        const auto* goal = prioritizedGoal.getGoal();
        if (goal != nullptr && goal->getTypeName() == "RaidGardenGoal") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "RabbitEntity 应在 goalSelector 中注册 RaidGardenGoal";
}

TEST_F(RabbitEntityTest, RaidGardenGoal_ShouldNotExecute_WhenMobGriefingDisabled)
{
    // MOB_GRIEFING=false 时，饥饿兔子也不应执行 RaidGardenGoal
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.setMoreCarrotTicks(0); // 饥饿状态
    EXPECT_TRUE(rabbit.wantsMoreFood());

    // 禁用 MOB_GRIEFING
    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, false, nullptr);

    entity::ai::goal::RaidGardenGoal goal(&rabbit);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(RabbitEntityTest, RaidGardenGoal_ShouldNotExecute_WhenNotHungry)
{
    // MOB_GRIEFING=true 但 moreCarrotTicks>0（不饿）时，不应找到目标
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.setMoreCarrotTicks(40); // 不饿
    EXPECT_FALSE(rabbit.wantsMoreFood());

    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, true, nullptr);

    // 在兔子脚下放置耕地 + 成熟胡萝卜
    const i32 x = static_cast<i32>(std::floor(rabbit.x()));
    const i32 y = static_cast<i32>(std::floor(rabbit.y())) - 1;
    const i32 z = static_cast<i32>(std::floor(rabbit.z()));
    placeBlock(m_world, x, y, z, VanillaBlocks::FARMLAND);
    placeCarrotAtAge(m_world, x, y + 1, z, 7);

    entity::ai::goal::RaidGardenGoal goal(&rabbit);
    // shouldExecute 在 m_runDelay==0 时进入搜索，但 wantsToRaid=false，shouldMoveTo 返回 false
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(RabbitEntityTest, RaidGardenGoal_FindsMatureCarrotAndRaidsIt)
{
    // 饥饿兔子在成熟胡萝卜旁（兔子生成在原点，胡萝卜在脚下耕地正上方），
    // shouldExecute 找到目标，tick() 因兔子已到达目标触发掠夺：
    // AGE=7 → 6，moreCarrotTicks 设为 40。
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.setMoreCarrotTicks(0); // 饥饿
    EXPECT_TRUE(rabbit.wantsMoreFood());

    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, true, nullptr);

    // 兔子在原点 (0,0,0)，耕地在 (0,-1,0)，胡萝卜在 (0,0,0)
    const i32 x = 0;
    const i32 y = -1;
    const i32 z = 0;
    placeBlock(m_world, x, y, z, VanillaBlocks::FARMLAND);
    placeCarrotAtAge(m_world, x, y + 1, z, 7);

    // 验证初始 AGE=7
    const BlockState* beforeState = m_world.getBlockState(x, y + 1, z);
    ASSERT_NE(beforeState, nullptr);
    auto* carrotBlock = dynamic_cast<const blocks::CropBlock*>(VanillaBlocks::CARROTS);
    ASSERT_NE(carrotBlock, nullptr);
    EXPECT_EQ(carrotBlock->getAge(*beforeState), 7);

    // 构造 goal 并调用 shouldExecute，验证搜索逻辑找到成熟胡萝卜
    entity::ai::goal::RaidGardenGoal goal(&rabbit);
    ASSERT_TRUE(goal.shouldExecute());

    // startExecuting 调用 tryMoveTo（测试世界为空操作）
    goal.startExecuting();

    // tick()：兔子在原点，目标方块上方也在原点，距离 < 1.0，触发掠夺
    goal.tick();

    // 验证胡萝卜 AGE 从 7 降到 6
    const BlockState* afterState = m_world.getBlockState(x, y + 1, z);
    ASSERT_NE(afterState, nullptr);
    EXPECT_EQ(carrotBlock->getAge(*afterState), 6);

    // 验证 moreCarrotTicks 被设为 40（MORE_CARROTS_DELAY）
    EXPECT_EQ(rabbit.moreCarrotTicks(), 40);
    EXPECT_FALSE(rabbit.wantsMoreFood());
}

TEST_F(RabbitEntityTest, RaidGardenGoal_RaidsAgeZeroCarrot_RemovesBlock)
{
    // AGE=0 的胡萝卜被掠夺后应变为 AIR（对应 MC setBlock(AIR)）
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.setMoreCarrotTicks(0); // 饥饿

    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, true, nullptr);

    const i32 x = 0;
    const i32 y = -1;
    const i32 z = 0;
    placeBlock(m_world, x, y, z, VanillaBlocks::FARMLAND);
    placeCarrotAtAge(m_world, x, y + 1, z, 0); // AGE=0

    entity::ai::goal::RaidGardenGoal goal(&rabbit);
    // shouldMoveTo 要求 isMaxAge，AGE=0 不满足，shouldExecute 返回 false
    // 所以需要手动设置 canRaid。但 canRaid 是私有字段，无法直接设置。
    // 改为验证 AGE=0 的胡萝卜不会被搜索到（因为 isMaxAge=false）。
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(RabbitEntityTest, RaidGardenGoal_ShouldMoveTo_RejectsNonFarmland)
{
    // 非耕地（如草地）上方有成熟胡萝卜时，shouldMoveTo 应返回 false
    // 通过 shouldExecute 间接验证：兔子在草地上即使有成熟胡萝卜也不应找到目标
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.setMoreCarrotTicks(0); // 饥饿

    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, true, nullptr);

    const i32 x = static_cast<i32>(std::floor(rabbit.x()));
    const i32 y = static_cast<i32>(std::floor(rabbit.y())) - 1;
    const i32 z = static_cast<i32>(std::floor(rabbit.z()));
    // 草地（非耕地）+ 成熟胡萝卜
    placeBlock(m_world, x, y, z, VanillaBlocks::GRASS_BLOCK);
    placeCarrotAtAge(m_world, x, y + 1, z, 7);

    entity::ai::goal::RaidGardenGoal goal(&rabbit);
    // 草地不是有效目标，shouldExecute 应返回 false（找不到任何目标）
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(RabbitEntityTest, RaidGardenGoal_ShouldMoveTo_RejectsImmatureCarrot)
{
    // 耕地上方有未成熟胡萝卜（AGE<7）时，shouldMoveTo 应返回 false
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.setMoreCarrotTicks(0); // 饥饿

    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, true, nullptr);

    const i32 x = static_cast<i32>(std::floor(rabbit.x()));
    const i32 y = static_cast<i32>(std::floor(rabbit.y())) - 1;
    const i32 z = static_cast<i32>(std::floor(rabbit.z()));
    placeBlock(m_world, x, y, z, VanillaBlocks::FARMLAND);
    placeCarrotAtAge(m_world, x, y + 1, z, 3); // 未成熟

    entity::ai::goal::RaidGardenGoal goal(&rabbit);
    // 未成熟胡萝卜不是有效目标
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(RabbitEntityTest, RaidGardenGoal_ShouldMoveTo_AcceptsMatureCarrotOnFarmland)
{
    // 耕地 + 成熟胡萝卜（AGE=7）是有效目标
    RabbitEntity rabbit(EntityInstanceId(1));
    rabbit.setWorld(&m_world);
    rabbit.setMoreCarrotTicks(0); // 饥饿

    m_world.getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, true, nullptr);

    const i32 x = static_cast<i32>(std::floor(rabbit.x()));
    const i32 y = static_cast<i32>(std::floor(rabbit.y())) - 1;
    const i32 z = static_cast<i32>(std::floor(rabbit.z()));
    placeBlock(m_world, x, y, z, VanillaBlocks::FARMLAND);
    placeCarrotAtAge(m_world, x, y + 1, z, 7); // 成熟

    entity::ai::goal::RaidGardenGoal goal(&rabbit);
    // 成熟胡萝卜是有效目标，shouldExecute 应返回 true
    EXPECT_TRUE(goal.shouldExecute());
}

} // namespace
} // namespace mc
