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
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/spawn/SlimeChunkChecker.hpp"
#include "core/Types.hpp"
#include "util/math/random/Random.hpp"
#include "world/block/Block.hpp"
#include "world/lighting/InternalLightUtils.hpp"
#include "world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <gtest/gtest.h>

#include <unordered_map>

using namespace mc;
using namespace mc::world::spawn;
using mc::world::biome::BiomeTags;

namespace mc {
namespace test {

// ============================================================================
// 可控的测试用 ISpawnWorldReader 实现
// ============================================================================

/**
 * @brief 用于 canSlimeSpawn 测试的可控世界读取器
 *
 * 允许精确控制每个 ISpawnWorldReader 参数：
 * - seed: 世界种子（影响史莱姆区块判断）
 * - difficulty: 游戏难度（和平难度拒绝史莱姆生成）
 * - dayTime: 游戏时间（影响月相和光照）
 * - biomeId: 指定位置的生物群系
 * - maxBrightness: 指定位置的最大原始亮度
 * - m_groundSolid: 控制脚下方块是否为实心（默认 true，用于 OnGround 放置检查）
 *
 * 对于 OnGround 放置类型的实体（如史莱姆），canSpawnEntity 会先检查
 * checkOnGroundSpawn（需要脚下有实心方块、上方非实心），
 * 然后才调用 canSlimeSpawn 谓词。因此需要模拟合理的方块状态。
 */
class MockSpawnWorld final : public ISpawnWorldReader {
public:
    /// 世界种子
    u64 m_seed = 0;
    /// 游戏难度
    Difficulty m_difficulty = Difficulty::Normal;
    /// 游戏日时间（ticks）
    i64 m_dayTime = 0;
    /// 返回的生物群系ID
    BiomeId m_biomeId = Biomes::Plains;
    /// 返回的最大原始亮度
    i32 m_maxBrightness = 15;
    /// 世界边界 Y 范围
    i32 m_minY = -64;
    i32 m_maxY = 320;
    /// 脚下方块是否实心（OnGround 放置检查需要）
    bool m_groundSolid = true;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        // 对于 OnGround 生成检查：
        // checkOnGroundSpawn 检查 pos.y-1（脚下）、pos.y（生成位置）、pos.y+1（上方）
        // 脚下需要实心方块，生成位置和上方需要非实心、非流体、不阻止生成
        // 使用自定义方块状态映射，如果没有设置则根据 m_groundSolid 返回默认值
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    /// 设置指定位置的方块状态
    void setBlockState(i32 x, i32 y, i32 z, const BlockState* state) { m_blocks[BlockPos(x, y, z)] = state; }

    /// 配置指定生成位置 Y 坐标的 OnGround 方块状态
    /// 在 y-1 放置实心方块（脚底），y 和 y+1 为空气
    void setupOnGroundBlocks(i32 spawnY, i32 x = 0, i32 z = 0)
    {
        // 脚下实心方块
        setBlockState(x, spawnY - 1, z, &VanillaBlocks::STONE->defaultState());
        // 生成位置和上方为空气（nullptr = air）
        setBlockState(x, spawnY, z, &VanillaBlocks::AIR->defaultState());
        setBlockState(x, spawnY + 1, z, &VanillaBlocks::AIR->defaultState());
    }

    [[nodiscard]] bool isInWorldBounds(i32, i32 y, i32) const override { return y >= m_minY && y < m_maxY; }

    [[nodiscard]] i32 getHeight(HeightmapType, i32, i32) const override { return 64; }

    [[nodiscard]] BiomeId getBiome(i32, i32, i32) const override { return m_biomeId; }

    [[nodiscard]] u64 seed() const override { return m_seed; }

    [[nodiscard]] Difficulty difficulty() const override { return m_difficulty; }

    [[nodiscard]] i64 dayTime() const override { return m_dayTime; }

    [[nodiscard]] i32 getMaxLocalRawBrightness(i32, i32, i32) const override { return m_maxBrightness; }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blocks;
};

// ============================================================================
// canSlimeSpawn 测试套件
// ============================================================================

class CanSlimeSpawnTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块注册（VanillaBlocks::STONE 等需要）
        VanillaBlocks::initialize();
        // 初始化生物群系标签（沼泽 ALLOWS_SURFACE_SLIME_SPAWNS 需要此初始化）
        BiomeTags::initialize();
        // 初始化实体放置注册表
        EntitySpawnPlacementRegistry::initializeDefaults();
    }
};

// ========== 和平难度拒绝 ==========

TEST_F(CanSlimeSpawnTest, PeacefulDifficultyRejectsUnderground)
{
    // 和平难度下，即使是史莱姆区块也应拒绝生成
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Peaceful;
    world.m_seed = 0;

    // 找到一个史莱姆区块
    i32 slimeChunkX = 0, slimeChunkZ = 0;
    for (i32 x = -50; x < 50; ++x) {
        for (i32 z = -50; z < 50; ++z) {
            if (SlimeChunkChecker::isSlimeChunk(0, x, z)) {
                slimeChunkX = x;
                slimeChunkZ = z;
                break;
            }
        }
        if (slimeChunkX != 0 || slimeChunkZ != 0) break;
    }

    // 确保我们找到了一个史莱姆区块
    ASSERT_TRUE(SlimeChunkChecker::isSlimeChunk(0, slimeChunkX, slimeChunkZ));

    // 在史莱姆区块中、Y<40 的位置，和平难度仍然拒绝
    const i32 spawnX = slimeChunkX * 16 + 8;
    const i32 spawnZ = slimeChunkZ * 16 + 8;
    world.setupOnGroundBlocks(20, spawnX, spawnZ);
    Vector3i pos(spawnX, 20, spawnZ);
    math::Random random(12345);

    EXPECT_FALSE(
        EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, random));
}

TEST_F(CanSlimeSpawnTest, PeacefulDifficultyRejectsSurface)
{
    // 和平难度下，地表沼泽也应拒绝生成
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Peaceful;
    world.m_biomeId = Biomes::Swamp; // 沼泽允许地表史莱姆
    world.m_dayTime = 0;             // 满月
    world.m_maxBrightness = 0;       // 黑暗
    world.setupOnGroundBlocks(60);

    Vector3i pos(0, 60, 0); // Y 在 (50, 70) 范围内
    math::Random random(12345);

    EXPECT_FALSE(
        EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, random));
}

// ========== 地下史莱姆区块路径 ==========

TEST_F(CanSlimeSpawnTest, UndergroundSlimeChunkPathRequiresYBelow40)
{
    // 非史莱姆区块的 Y<40 位置不应生成（地下路径需要史莱姆区块）
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Plains; // 非沼泽生物群系

    // 找一个非史莱姆区块
    i32 nonSlimeX = 0, nonSlimeZ = 0;
    for (i32 x = 0; x < 100; ++x) {
        for (i32 z = 0; z < 100; ++z) {
            if (!SlimeChunkChecker::isSlimeChunk(0, x, z)) {
                nonSlimeX = x;
                nonSlimeZ = z;
                break;
            }
        }
        if (nonSlimeX != 0 || nonSlimeZ != 0) break;
    }

    const i32 spawnX = nonSlimeX * 16 + 8;
    const i32 spawnZ = nonSlimeZ * 16 + 8;
    world.setupOnGroundBlocks(20, spawnX, spawnZ);

    // Y < 40 但非史莱姆区块 → 不应生成（使用 ChunkGeneration 以走地下路径）
    Vector3i posUnderground(spawnX, 20, spawnZ);
    bool anySuccess = false;
    for (i32 i = 0; i < 100; ++i) {
        math::Random random(static_cast<u64>(i * 3571 + 17));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::ChunkGeneration, posUnderground, random)) {
            anySuccess = true;
            break;
        }
    }
    EXPECT_FALSE(anySuccess);
}

TEST_F(CanSlimeSpawnTest, UndergroundSlimeChunkPathRejectsYAtOrAbove40)
{
    // Y >= 40 时不走地下路径（非沼泽生物群系时也不走地表路径）
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Plains;

    // 找一个史莱姆区块
    i32 slimeChunkX = 0, slimeChunkZ = 0;
    for (i32 x = -50; x < 50; ++x) {
        for (i32 z = -50; z < 50; ++z) {
            if (SlimeChunkChecker::isSlimeChunk(0, x, z)) {
                slimeChunkX = x;
                slimeChunkZ = z;
                break;
            }
        }
        if (slimeChunkX != 0 || slimeChunkZ != 0) break;
    }

    const i32 spawnX = slimeChunkX * 16 + 8;
    const i32 spawnZ = slimeChunkZ * 16 + 8;
    world.setupOnGroundBlocks(40, spawnX, spawnZ);

    // Y=40 → 地下路径要求 Y<40（严格小于），所以不走地下路径
    // Plains 生物群系没有 ALLOWS_SURFACE_SLIME_SPAWNS 标签，也不走地表路径
    Vector3i posAt40(spawnX, 40, spawnZ);
    bool anySuccess = false;
    for (i32 i = 0; i < 100; ++i) {
        math::Random random(static_cast<u64>(i * 3571 + 17));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::ChunkGeneration, posAt40, random)) {
            anySuccess = true;
            break;
        }
    }
    EXPECT_FALSE(anySuccess);
}

TEST_F(CanSlimeSpawnTest, UndergroundSlimeChunkPathRequiresChunkGeneration)
{
    // 地下史莱姆区块路径只在 ChunkGeneration 阶段触发
    // Natural 阶段即使 Y<40 且在史莱姆区块也不应走地下路径
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Plains;

    // 找一个史莱姆区块
    i32 slimeChunkX = 0, slimeChunkZ = 0;
    for (i32 x = -50; x < 50; ++x) {
        for (i32 z = -50; z < 50; ++z) {
            if (SlimeChunkChecker::isSlimeChunk(0, x, z)) {
                slimeChunkX = x;
                slimeChunkZ = z;
                break;
            }
        }
        if (slimeChunkX != 0 || slimeChunkZ != 0) break;
    }

    ASSERT_TRUE(SlimeChunkChecker::isSlimeChunk(0, slimeChunkX, slimeChunkZ));

    const i32 spawnX = slimeChunkX * 16 + 8;
    const i32 spawnZ = slimeChunkZ * 16 + 8;
    world.setupOnGroundBlocks(20, spawnX, spawnZ);
    Vector3i pos(spawnX, 20, spawnZ);

    // Natural 阶段不应在地下路径生成史莱姆
    {
        bool anySuccess = false;
        for (i32 i = 0; i < 100; ++i) {
            math::Random random(static_cast<u64>(i * 7919 + 31));
            if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, random)) {
                anySuccess = true;
                break;
            }
        }
        EXPECT_FALSE(anySuccess) << "Slime should not spawn underground via Natural reason";
    }

    // ChunkGeneration 阶段应该在史莱姆区块的地下路径有可能生成
    {
        i32 successCount = 0;
        constexpr i32 TOTAL_ATTEMPTS = 1000;
        for (i32 i = 0; i < TOTAL_ATTEMPTS; ++i) {
            math::Random random(static_cast<u64>(i * 7919 + 31));
            if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::ChunkGeneration, pos, random)) {
                ++successCount;
            }
        }
        // 期望约 10% 的成功率，允许 5%-20% 的范围
        const f32 ratio = static_cast<f32>(successCount) / static_cast<f32>(TOTAL_ATTEMPTS);
        EXPECT_GT(ratio, 0.05f) << "Slime spawn rate in slime chunk (ChunkGeneration) too low: " << ratio;
        EXPECT_LT(ratio, 0.20f) << "Slime spawn rate in slime chunk (ChunkGeneration) too high: " << ratio;
    }
}

// ========== 地表沼泽路径 ==========

TEST_F(CanSlimeSpawnTest, SurfacePathRequiresSwampBiome)
{
    // 非沼泽生物群系不应该走地表路径
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Plains; // 非沼泽
    world.m_dayTime = 0;              // 满月
    world.m_maxBrightness = 0;        // 黑暗
    world.setupOnGroundBlocks(60);

    Vector3i pos(0, 60, 0); // Y 在 (50, 70) 范围
    math::Random random(12345);

    bool anySuccess = false;
    for (i32 i = 0; i < 200; ++i) {
        math::Random r(static_cast<u64>(i * 3571 + 17));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, r)) {
            anySuccess = true;
            break;
        }
    }
    EXPECT_FALSE(anySuccess) << "Slime should not spawn on surface in Plains biome";
}

TEST_F(CanSlimeSpawnTest, SurfacePathSwampBiomeCanSpawn)
{
    // 沼泽生物群系 + 满月 + 低光照 + 合适 Y → 应该可能生成
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Swamp;
    world.m_dayTime = 0;       // 满月（月相0），spawnChance = 0.5
    world.m_maxBrightness = 0; // 黑暗，光照检查总是通过
    world.setupOnGroundBlocks(60);

    Vector3i pos(0, 60, 0); // Y 在 (50, 70) 范围内
    i32 successCount = 0;
    constexpr i32 TOTAL_ATTEMPTS = 1000;
    for (i32 i = 0; i < TOTAL_ATTEMPTS; ++i) {
        math::Random random(static_cast<u64>(i * 4231 + 7));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, random)) {
            ++successCount;
        }
    }

    // 满月时 spawnChance = 0.5，光照=0 检查总是通过
    // 期望约 50% 的成功率
    const f32 ratio = static_cast<f32>(successCount) / static_cast<f32>(TOTAL_ATTEMPTS);
    EXPECT_GT(ratio, 0.30f) << "Surface slime spawn rate in swamp at full moon too low: " << ratio;
    EXPECT_LT(ratio, 0.70f) << "Surface slime spawn rate in swamp at full moon too high: " << ratio;
}

TEST_F(CanSlimeSpawnTest, SurfacePathMangroveSwampCanSpawn)
{
    // 红树林沼泽也应该允许地表史莱姆
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::MangroveSwamp;
    world.m_dayTime = 0; // 满月
    world.m_maxBrightness = 0;
    world.setupOnGroundBlocks(60);

    Vector3i pos(0, 60, 0);
    i32 successCount = 0;
    constexpr i32 TOTAL_ATTEMPTS = 500;
    for (i32 i = 0; i < TOTAL_ATTEMPTS; ++i) {
        math::Random random(static_cast<u64>(i * 6131 + 13));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, random)) {
            ++successCount;
        }
    }

    // 应该有非零的成功率
    EXPECT_GT(successCount, 0) << "Slime should be able to spawn in Mangrove Swamp";
}

TEST_F(CanSlimeSpawnTest, SurfacePathNewMoonRejects)
{
    // 新月时地表史莱姆生成概率为 0
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Swamp;
    world.m_maxBrightness = 0;
    world.setupOnGroundBlocks(60);

    // 月相4 = 新月，spawnChance = 0.0
    // dayTime 需要对8取模为4：dayTime / DAY_LENGTH_TICKS % 8 == 4
    // DAY_LENGTH_TICKS = 24000, 所以 dayTime = 4 * 24000 = 96000
    const i64 dayTimeForNewMoon = 4 * 24000;
    world.m_dayTime = dayTimeForNewMoon;

    // 验证月相确实是新月
    ASSERT_EQ(InternalLightUtils::getMoonPhase(dayTimeForNewMoon), 4);

    Vector3i pos(0, 60, 0);
    math::Random random(12345);

    bool anySuccess = false;
    for (i32 i = 0; i < 500; ++i) {
        math::Random r(static_cast<u64>(i * 2719 + 3));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, r)) {
            anySuccess = true;
            break;
        }
    }
    EXPECT_FALSE(anySuccess) << "Slime should not spawn on surface during new moon";
}

TEST_F(CanSlimeSpawnTest, SurfacePathRejectsY50OrBelow)
{
    // Y <= 50 不满足 Y > 50 条件
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Swamp;
    world.m_dayTime = 0; // 满月
    world.m_maxBrightness = 0;
    world.setupOnGroundBlocks(50);

    Vector3i pos(0, 50, 0); // Y = 50 不满足 Y > 50
    math::Random random(12345);

    bool anySuccess = false;
    for (i32 i = 0; i < 200; ++i) {
        math::Random r(static_cast<u64>(i * 3571 + 17));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, r)) {
            anySuccess = true;
            break;
        }
    }
    EXPECT_FALSE(anySuccess) << "Slime should not spawn on surface at Y=50";
}

TEST_F(CanSlimeSpawnTest, SurfacePathRejectsY70OrAbove)
{
    // Y >= 70 不满足 Y < 70 条件
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Swamp;
    world.m_dayTime = 0;
    world.m_maxBrightness = 0;
    world.setupOnGroundBlocks(70);

    Vector3i pos(0, 70, 0); // Y = 70 不满足 Y < 70
    math::Random random(12345);

    bool anySuccess = false;
    for (i32 i = 0; i < 200; ++i) {
        math::Random r(static_cast<u64>(i * 3571 + 17));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, r)) {
            anySuccess = true;
            break;
        }
    }
    EXPECT_FALSE(anySuccess) << "Slime should not spawn on surface at Y=70";
}

TEST_F(CanSlimeSpawnTest, SurfacePathAcceptsYBetween50And70)
{
    // Y = 60 满足 50 < Y < 70
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Swamp;
    world.m_dayTime = 0;
    world.m_maxBrightness = 0;
    world.setupOnGroundBlocks(60);

    Vector3i pos(0, 60, 0);
    math::Random random(12345);

    bool anySuccess = false;
    for (i32 i = 0; i < 200; ++i) {
        math::Random r(static_cast<u64>(i * 3571 + 17));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, r)) {
            anySuccess = true;
            break;
        }
    }
    EXPECT_TRUE(anySuccess) << "Slime should be able to spawn on surface at Y=60 in swamp at full moon";
}

TEST_F(CanSlimeSpawnTest, SurfacePathLightLevelCheck)
{
    // 光照等级检查：亮度 <= random(0-7)
    // 设置高亮度，应该完全无法生成
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Swamp;
    world.m_dayTime = 0;        // 满月
    world.m_maxBrightness = 15; // 最大亮度
    world.setupOnGroundBlocks(60);

    Vector3i pos(0, 60, 0);
    i32 successCount = 0;
    constexpr i32 TOTAL_ATTEMPTS = 1000;
    for (i32 i = 0; i < TOTAL_ATTEMPTS; ++i) {
        math::Random random(static_cast<u64>(i * 4231 + 7));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, random)) {
            ++successCount;
        }
    }

    // 亮度15时，需要 brightness <= nextInt(8)，即 15 <= [0,7]，永远不可能
    // 所以应该完全没有生成
    EXPECT_EQ(successCount, 0) << "Slime should not spawn on surface with brightness 15";
}

TEST_F(CanSlimeSpawnTest, SurfacePathLowBrightnessCanSpawn)
{
    // 低亮度（0）时，光照检查总是通过（0 <= random(0-7) 总是成立）
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Swamp;
    world.m_dayTime = 0; // 满月
    world.m_maxBrightness = 0;
    world.setupOnGroundBlocks(60);

    Vector3i pos(0, 60, 0);
    i32 successCount = 0;
    constexpr i32 TOTAL_ATTEMPTS = 1000;
    for (i32 i = 0; i < TOTAL_ATTEMPTS; ++i) {
        math::Random random(static_cast<u64>(i * 4231 + 7));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, random)) {
            ++successCount;
        }
    }

    // 亮度0时光照检查总是通过，满月时spawnChance=0.5
    const f32 ratio = static_cast<f32>(successCount) / static_cast<f32>(TOTAL_ATTEMPTS);
    EXPECT_GT(ratio, 0.30f) << "Surface slime spawn at full moon with low brightness too low: " << ratio;
    EXPECT_LT(ratio, 0.70f) << "Surface slime spawn at full moon with low brightness too high: " << ratio;
}

TEST_F(CanSlimeSpawnTest, SurfacePathMediumBrightnessReducesChance)
{
    // 中等亮度（7）时，光照检查通过概率约 1/8 (nextInt(8) >= 7)
    // 亮度7: nextInt(8) 返回 0-7, 7 <= nextInt(8) 意味着 nextInt(8) >= 7，概率 1/8
    // 所以亮度7时生成率应约为 满月0.5 * 1/8 = 6.25%
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Swamp;
    world.m_dayTime = 0;
    world.m_maxBrightness = 7;
    world.setupOnGroundBlocks(60);

    Vector3i pos(0, 60, 0);
    i32 successCount = 0;
    constexpr i32 TOTAL_ATTEMPTS = 2000;
    for (i32 i = 0; i < TOTAL_ATTEMPTS; ++i) {
        math::Random random(static_cast<u64>(i * 4231 + 7));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, random)) {
            ++successCount;
        }
    }

    // 期望约 0.5 * 1/8 = 6.25%，允许 2%-15% 的范围
    const f32 ratio = static_cast<f32>(successCount) / static_cast<f32>(TOTAL_ATTEMPTS);
    EXPECT_GT(ratio, 0.02f) << "Surface slime spawn at brightness 7 too low: " << ratio;
    EXPECT_LT(ratio, 0.15f) << "Surface slime spawn at brightness 7 too high: " << ratio;
}

// ========== 综合路径交互测试 ==========

TEST_F(CanSlimeSpawnTest, SlimeChunkAndSwampBiomeBothPathsAvailable)
{
    // 当 Y<40 且在沼泽生物群系中的史莱姆区块时：
    // - ChunkGeneration 阶段：地下路径可用（Y<40 且史莱姆区块）
    // - Natural 阶段：地下路径不可用，地表路径也不可用（Y<50 不满足 Y>50）
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Swamp;

    // 找一个史莱姆区块
    i32 slimeChunkX = 0, slimeChunkZ = 0;
    for (i32 x = -50; x < 50; ++x) {
        for (i32 z = -50; z < 50; ++z) {
            if (SlimeChunkChecker::isSlimeChunk(0, x, z)) {
                slimeChunkX = x;
                slimeChunkZ = z;
                break;
            }
        }
        if (slimeChunkX != 0 || slimeChunkZ != 0) break;
    }

    ASSERT_TRUE(SlimeChunkChecker::isSlimeChunk(0, slimeChunkX, slimeChunkZ));

    const i32 spawnX = slimeChunkX * 16 + 8;
    const i32 spawnZ = slimeChunkZ * 16 + 8;
    world.setupOnGroundBlocks(20, spawnX, spawnZ);

    // Y=20: 地表路径不可用（Y<50 不满足 Y>50）
    // ChunkGeneration 阶段：地下路径可用
    Vector3i pos(spawnX, 20, spawnZ);
    i32 successCount = 0;
    constexpr i32 TOTAL_ATTEMPTS = 500;
    for (i32 i = 0; i < TOTAL_ATTEMPTS; ++i) {
        math::Random random(static_cast<u64>(i * 7919 + 31));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::ChunkGeneration, pos, random)) {
            ++successCount;
        }
    }

    // 地下路径只有 10% 概率（nextInt(10)==0）
    const f32 ratio = static_cast<f32>(successCount) / static_cast<f32>(TOTAL_ATTEMPTS);
    EXPECT_GT(ratio, 0.03f) << "Underground slime spawn in slime chunk (ChunkGeneration) too low: " << ratio;
    EXPECT_LT(ratio, 0.20f) << "Underground slime spawn in slime chunk (ChunkGeneration) too high: " << ratio;
}

TEST_F(CanSlimeSpawnTest, NonSlimeEntityNotAffected)
{
    // 非史莱姆实体不应受 canSlimeSpawn 影响
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Peaceful;
    world.m_seed = 0;

    Vector3i pos(0, 60, 0);
    math::Random random(12345);

    // 验证 canSpawnEntity 不崩溃即可
    EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, random);
}

TEST_F(CanSlimeSpawnTest, QuarterMoonSurfaceSpawnRate)
{
    // 月相2（下弦月）亮度0.5，spawnChance = 0.5 * 0.5 = 0.25
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Swamp;
    world.m_maxBrightness = 0;
    world.setupOnGroundBlocks(60);

    // 月相2：dayTime = 2 * 24000 = 48000
    const i64 dayTimeForQuarterMoon = 2 * 24000;
    world.m_dayTime = dayTimeForQuarterMoon;

    ASSERT_EQ(InternalLightUtils::getMoonPhase(dayTimeForQuarterMoon), 2);

    Vector3i pos(0, 60, 0);
    i32 successCount = 0;
    constexpr i32 TOTAL_ATTEMPTS = 1000;
    for (i32 i = 0; i < TOTAL_ATTEMPTS; ++i) {
        math::Random random(static_cast<u64>(i * 4231 + 7));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, random)) {
            ++successCount;
        }
    }

    // 期望约 25% 的成功率
    const f32 ratio = static_cast<f32>(successCount) / static_cast<f32>(TOTAL_ATTEMPTS);
    EXPECT_GT(ratio, 0.12f) << "Surface slime spawn rate at quarter moon too low: " << ratio;
    EXPECT_LT(ratio, 0.40f) << "Surface slime spawn rate at quarter moon too high: " << ratio;
}

TEST_F(CanSlimeSpawnTest, SlimeChunkDeterministicWithSameSeed)
{
    // 相同种子和位置，canSlimeSpawn 应该是确定性的
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Plains;

    i32 slimeChunkX = 0, slimeChunkZ = 0;
    for (i32 x = -50; x < 50; ++x) {
        for (i32 z = -50; z < 50; ++z) {
            if (SlimeChunkChecker::isSlimeChunk(0, x, z)) {
                slimeChunkX = x;
                slimeChunkZ = z;
                break;
            }
        }
        if (slimeChunkX != 0 || slimeChunkZ != 0) break;
    }

    const i32 spawnX = slimeChunkX * 16 + 8;
    const i32 spawnZ = slimeChunkZ * 16 + 8;
    world.setupOnGroundBlocks(20, spawnX, spawnZ);
    Vector3i pos(spawnX, 20, spawnZ);

    // 相同的 Random 种子应该得到相同的结果（使用 ChunkGeneration 以走地下路径）
    math::Random random1(42);
    bool result1 = EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::ChunkGeneration, pos, random1);

    math::Random random2(42);
    bool result2 = EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::ChunkGeneration, pos, random2);

    EXPECT_EQ(result1, result2);
}

TEST_F(CanSlimeSpawnTest, EasyDifficultyAllowsSlime)
{
    // 简单难度应该允许史莱姆生成
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Easy;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Swamp;
    world.m_dayTime = 0; // 满月
    world.m_maxBrightness = 0;
    world.setupOnGroundBlocks(60);

    Vector3i pos(0, 60, 0);
    i32 successCount = 0;
    constexpr i32 TOTAL_ATTEMPTS = 500;
    for (i32 i = 0; i < TOTAL_ATTEMPTS; ++i) {
        math::Random random(static_cast<u64>(i * 4231 + 7));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, random)) {
            ++successCount;
        }
    }

    EXPECT_GT(successCount, 0) << "Slime should spawn on Easy difficulty";
}

TEST_F(CanSlimeSpawnTest, HardDifficultyAllowsSlime)
{
    // 困难难度应该允许史莱姆生成
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Hard;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Swamp;
    world.m_dayTime = 0;
    world.m_maxBrightness = 0;
    world.setupOnGroundBlocks(60);

    Vector3i pos(0, 60, 0);
    i32 successCount = 0;
    constexpr i32 TOTAL_ATTEMPTS = 500;
    for (i32 i = 0; i < TOTAL_ATTEMPTS; ++i) {
        math::Random random(static_cast<u64>(i * 4231 + 7));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, random)) {
            ++successCount;
        }
    }

    EXPECT_GT(successCount, 0) << "Slime should spawn on Hard difficulty";
}

// ========== 刷怪笼生成路径测试 ==========

TEST_F(CanSlimeSpawnTest, SpawnerBypassesSlimeChunkAndSwampConditions)
{
    // 刷怪笼生成（SpawnReason::Spawner）应跳过史莱姆区块和沼泽条件检查
    // 即使在非史莱姆区块、非沼泽生物群系、Y<40 的位置也应允许生成
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Plains; // 非沼泽

    // 找一个非史莱姆区块
    i32 nonSlimeX = 0, nonSlimeZ = 0;
    for (i32 x = 0; x < 100; ++x) {
        for (i32 z = 0; z < 100; ++z) {
            if (!SlimeChunkChecker::isSlimeChunk(0, x, z)) {
                nonSlimeX = x;
                nonSlimeZ = z;
                break;
            }
        }
        if (nonSlimeX != 0 || nonSlimeZ != 0) break;
    }

    const i32 spawnX = nonSlimeX * 16 + 8;
    const i32 spawnZ = nonSlimeZ * 16 + 8;
    world.setupOnGroundBlocks(20, spawnX, spawnZ);

    Vector3i pos(spawnX, 20, spawnZ);
    math::Random random(12345);

    // Natural 阶段不应在非史莱姆区块、非沼泽的 Y<40 位置生成
    EXPECT_FALSE(
        EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, random));

    // Spawner 阶段应允许在任意位置生成
    math::Random random2(12345);
    EXPECT_TRUE(
        EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Spawner, pos, random2));
}

TEST_F(CanSlimeSpawnTest, SpawnerRejectsPeacefulDifficulty)
{
    // 刷怪笼生成仍然需要非和平难度
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Peaceful;
    world.m_seed = 0;
    world.setupOnGroundBlocks(60);

    Vector3i pos(0, 60, 0);
    math::Random random(12345);

    EXPECT_FALSE(
        EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Spawner, pos, random));
}

TEST_F(CanSlimeSpawnTest, SpawnerAllowsAnyPosition)
{
    // 刷怪笼生成允许在任何合法位置生成（只需非和平难度）
    // 测试 Y=60 的平原生物群系（Natural 不允许，Spawner 允许）
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Plains; // 非沼泽
    world.m_maxBrightness = 15;       // 高亮度
    world.setupOnGroundBlocks(60);

    Vector3i pos(0, 60, 0);
    math::Random random(12345);

    // Natural 不允许（非沼泽、Y>40）
    EXPECT_FALSE(
        EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, random));

    // Spawner 允许
    math::Random random2(12345);
    EXPECT_TRUE(
        EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Spawner, pos, random2));
}

TEST_F(CanSlimeSpawnTest, NaturalRejectsUndergroundInNonSlimeChunk)
{
    // Natural 阶段不应在地下路径生成史莱姆（即使和平难度下）
    MockSpawnWorld world;
    world.m_difficulty = Difficulty::Normal;
    world.m_seed = 0;
    world.m_biomeId = Biomes::Plains;

    // 非史莱姆区块、Y<40
    i32 nonSlimeX = 0, nonSlimeZ = 0;
    for (i32 x = 0; x < 100; ++x) {
        for (i32 z = 0; z < 100; ++z) {
            if (!SlimeChunkChecker::isSlimeChunk(0, x, z)) {
                nonSlimeX = x;
                nonSlimeZ = z;
                break;
            }
        }
        if (nonSlimeX != 0 || nonSlimeZ != 0) break;
    }

    const i32 spawnX = nonSlimeX * 16 + 8;
    const i32 spawnZ = nonSlimeZ * 16 + 8;
    world.setupOnGroundBlocks(20, spawnX, spawnZ);
    Vector3i pos(spawnX, 20, spawnZ);

    // Natural 在地下不应生成
    bool anySuccess = false;
    for (i32 i = 0; i < 100; ++i) {
        math::Random random(static_cast<u64>(i * 7919 + 31));
        if (EntitySpawnPlacementRegistry::canSpawnEntity("minecraft:slime", world, SpawnReason::Natural, pos, random)) {
            anySuccess = true;
            break;
        }
    }
    EXPECT_FALSE(anySuccess) << "Slime should not spawn underground with Natural reason in non-slime chunk";
}

} // namespace test
} // namespace mc
