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

// 表面规则正确性测试
// 验证 SurfaceRulesFactory 生成的表面规则与 MC 1.21.11 原版行为一致。
// 主要测试点：
//   1. 规则顺序正确性（SequenceRule 短路求值，顺序至关重要）
//   2. onFloor/underFloor 条件语义差异
//   3. 特定生物群系表面方块正确性
//   4. 深板岩/基岩层位置
//   5. 各维度基础表面行为

#include <gtest/gtest.h>

#include "common/core/Constants.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/density/Beardifier.hpp"

#include <iostream>
#include <memory>
#include <unordered_set>

namespace mc {
namespace {

// ============================================================================
// 测试夹具
// ============================================================================

class SurfaceRuleParityTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        fluid::FluidRegistry::instance().initialize();
    }

    struct GeneratedChunk {
        std::vector<std::unique_ptr<ChunkPrimer>> ownedChunks;
        std::unique_ptr<WorldGenRegion> region;
        std::unique_ptr<NoiseChunkGenerator> generator;
        ChunkPrimer* centerChunk = nullptr;
    };

    /**
     * @brief 生成主世界地形（3x3 区块区域）
     *
     * @param seed 世界种子
     * @param cx 中心区块 X
     * @param cz 中心区块 Z
     * @param radius 区域半径（0=单区块，1=3x3）
     */
    static std::unique_ptr<GeneratedChunk> generateOverworld(u64 seed, ChunkCoord cx, ChunkCoord cz, i32 radius = 1)
    {
        auto result = std::make_unique<GeneratedChunk>();
        const i32 diameter = radius * 2 + 1;

        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, seed);
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        result->generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));

        std::vector<IChunk*> chunkPtrs;
        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                auto primer = std::make_unique<ChunkPrimer>(cx + dx, cz + dz);
                chunkPtrs.push_back(primer.get());
                result->ownedChunks.push_back(std::move(primer));
            }
        }
        result->centerChunk = dynamic_cast<ChunkPrimer*>(chunkPtrs[static_cast<size_t>((radius * diameter) + radius)]);

        result->region = std::make_unique<WorldGenRegion>(cx, cz, radius, std::move(chunkPtrs), 0);

        // 对区域中所有区块执行生成管线：biomes -> noise -> surface
        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                size_t idx = static_cast<size_t>((dz + radius) * diameter + (dx + radius));
                ChunkPrimer* chunk = result->ownedChunks[idx].get();
                result->generator->generateBiomes(*result->region, *chunk);
                result->generator->generateNoise(*result->region, *chunk);
                result->generator->buildSurface(*result->region, *chunk);
            }
        }

        return result;
    }

    /**
     * @brief 生成下界地形
     */
    static std::unique_ptr<GeneratedChunk> generateNether(u64 seed, ChunkCoord cx, ChunkCoord cz)
    {
        auto result = std::make_unique<GeneratedChunk>();
        const i32 radius = 1;
        const i32 diameter = radius * 2 + 1;

        auto settings = DimensionSettings::nether();
        auto randomState = world::gen::RandomState::create(settings, seed);
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createNether(*randomState);
        result->generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));

        std::vector<IChunk*> chunkPtrs;
        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                auto primer = std::make_unique<ChunkPrimer>(cx + dx, cz + dz);
                chunkPtrs.push_back(primer.get());
                result->ownedChunks.push_back(std::move(primer));
            }
        }
        result->centerChunk = dynamic_cast<ChunkPrimer*>(chunkPtrs[static_cast<size_t>((radius * diameter) + radius)]);

        result->region = std::make_unique<WorldGenRegion>(cx, cz, radius, std::move(chunkPtrs), -1);

        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                size_t idx = static_cast<size_t>((dz + radius) * diameter + (dx + radius));
                ChunkPrimer* chunk = result->ownedChunks[idx].get();
                result->generator->generateBiomes(*result->region, *chunk);
                result->generator->generateNoise(*result->region, *chunk);
                result->generator->buildSurface(*result->region, *chunk);
            }
        }

        return result;
    }

    /**
     * @brief 获取地表方块（使用 WorldSurfaceWG 高度图）
     */
    static const BlockState* getSurfaceBlock(const ChunkPrimer& chunk, i32 x, i32 z)
    {
        const i32 surfaceY = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
        if (surfaceY < mc::world::MIN_BUILD_HEIGHT) {
            return nullptr;
        }
        return chunk.getBlockState(x, surfaceY, z);
    }

    /**
     * @brief 检查方块是否为特定 VanillaBlock
     */
    static bool isBlock(const BlockState* state, const Block* block) { return state != nullptr && state->is(block); }
};

// 判断生物群系是否为海洋变体。海洋生物群系的海床不应用默认 onFloor 表面规则
// （原版 onFloor 顶层规则被 waterBlockCheck(-1,0) 包裹：方块上方是水时跳过草/泥土，
// 海床保持 STONE）。因此草方块断言只应针对陆地生物群系列，否则会误报"表面规则 bug"。
bool _isOceanBiome(BiomeId id)
{
    using namespace world::biome::Biomes;
    switch (id) {
        case Ocean:
        case DeepOcean:
        case WarmOcean:
        case LukewarmOcean:
        case ColdOcean:
        case FrozenOcean:
        case DeepWarmOcean:
        case DeepLukewarmOcean:
        case DeepColdOcean:
        case DeepFrozenOcean:
            return true;
        default:
            return false;
    }
}

// ============================================================================
// 深板岩层测试
// ============================================================================

/**
 * @brief 深板岩层位于 abovePreliminarySurface 之后
 *
 * MC 源码中 deepslate 规则在 abovePreliminarySurface 之后，
 * 意味着在 abovePreliminarySurface 为真的位置，表面规则优先生效。
 * 只有在表面规则未匹配（返回 nullptr）时，深板岩才会被应用。
 * 如果 deepslate 在 abovePreliminarySurface 之前，地表附近 Y=0~8 的
 * 草方块/泥土/沙子等表面方块都会被错误地替换为深板岩。
 */
TEST_F(SurfaceRuleParityTest, DeepslateDoesNotOverrideSurfaceBlocks)
{
    constexpr u64 seed = 42;
    auto result = generateOverworld(seed, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    const auto& chunk = *result->centerChunk;

    // 检查地表方块不应该是深板岩（正常地形中地表远高于深板岩过渡区）
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const i32 surfaceY = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            if (surfaceY < mc::world::MIN_BUILD_HEIGHT) {
                continue;
            }
            const BlockState* surfaceBlock = chunk.getBlockState(x, surfaceY, z);
            // 地表方块不应是深板岩
            EXPECT_FALSE(isBlock(surfaceBlock, VanillaBlocks::DEEPSLATE))
                << "Surface block at (" << x << ", " << surfaceY << ", " << z << ") is deepslate";
        }
    }
}

/**
 * @brief 深板岩在 Y=0~8 过渡区确实存在
 *
 * 验证深板岩层在低 Y 坐标处正确生成。
 */
TEST_F(SurfaceRuleParityTest, DeepslateExistsAtLowY)
{
    constexpr u64 seed = 42;
    auto result = generateOverworld(seed, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    const auto& chunk = *result->centerChunk;
    bool foundDeepslate = false;

    // Y=-64 到 0 之间应该有深板岩
    for (i32 x = 0; x < 16 && !foundDeepslate; ++x) {
        for (i32 z = 0; z < 16 && !foundDeepslate; ++z) {
            for (i32 y = mc::world::MIN_BUILD_HEIGHT; y <= 0; ++y) {
                if (isBlock(chunk.getBlockState(x, y, z), VanillaBlocks::DEEPSLATE)) {
                    foundDeepslate = true;
                    break;
                }
            }
        }
    }

    EXPECT_TRUE(foundDeepslate) << "Deepslate should exist at low Y coordinates";
}

// ============================================================================
// 基岩层测试
// ============================================================================

/**
 * @brief 基岩层在最底层
 *
 * MC 源码中基岩规则在序列的最前面（优先级最高），
 * 应在 Y=-64 附近生成基岩层。
 */
TEST_F(SurfaceRuleParityTest, BedrockAtBottomLayer)
{
    constexpr u64 seed = 42;
    auto result = generateOverworld(seed, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    const auto& chunk = *result->centerChunk;
    i32 bedrockCount = 0;

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            if (isBlock(chunk.getBlockState(x, mc::world::MIN_BUILD_HEIGHT, z), VanillaBlocks::BEDROCK)) {
                ++bedrockCount;
            }
        }
    }

    // 底层应该大量基岩（不是全部，因为 verticalGradient 有随机性）
    EXPECT_GT(bedrockCount, 0) << "Bedrock should exist at the bottom layer";
}

// ============================================================================
// 草方块/地表层测试
// ============================================================================

/**
 * @brief 主世界地表应生成草方块
 *
 * onFloor + waterBlockCheck(-1, 0) 规则必须在 underFloor 材料层规则之前，
 * 否则地表会被错误替换为泥土。此测试验证规则顺序修复后的效果。
 */
TEST_F(SurfaceRuleParityTest, Overworld_SurfaceHasGrassBlock)
{
    auto result = generateOverworld(42, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    const auto& chunk = *result->centerChunk;
    i32 grassCount = 0;

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const i32 surfaceY = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            if (surfaceY < mc::world::MIN_BUILD_HEIGHT) {
                continue;
            }
            if (isBlock(chunk.getBlockState(x, surfaceY, z), VanillaBlocks::GRASS_BLOCK)) {
                ++grassCount;
            }
        }
    }

    EXPECT_GT(grassCount, 0) << "Overworld surface should have grass blocks";
}

/**
 * @brief 草方块下方应是泥土层
 *
 * 草方块下方（地表第二层）通常是泥土，这是 underFloor 材料层规则的默认行为。
 */
TEST_F(SurfaceRuleParityTest, Overworld_DirtBelowGrass)
{
    auto result = generateOverworld(42, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    const auto& chunk = *result->centerChunk;
    i32 grassWithDirtBelow = 0;
    i32 totalGrass = 0;

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const i32 surfaceY = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            if (surfaceY < mc::world::MIN_BUILD_HEIGHT || surfaceY <= mc::world::MIN_BUILD_HEIGHT) {
                continue;
            }
            if (!isBlock(chunk.getBlockState(x, surfaceY, z), VanillaBlocks::GRASS_BLOCK)) {
                continue;
            }
            ++totalGrass;
            // 草方块下方一格应为泥土
            const BlockState* below = chunk.getBlockState(x, surfaceY - 1, z);
            if (isBlock(below, VanillaBlocks::DIRT)) {
                ++grassWithDirtBelow;
            }
        }
    }

    if (totalGrass > 0) {
        // 大部分草方块下方应该是泥土（允许少量例外，如水下等特殊情况）
        EXPECT_GT(grassWithDirtBelow, 0) << "Most grass blocks should have dirt below them";
    }
}

/**
 * @brief 草方块不在 underFloor 层错误生成
 *
 * MC 源码中 underFloor 默认规则只有 DIRT，没有 grass/waterBlockCheck 分裂。
 * 删除了 C++ 代码中多出的 standalone underFloor grass/dirt 规则。
 * 此测试验证地下层（地表下2~4格）主要是泥土而非草方块。
 */
TEST_F(SurfaceRuleParityTest, Overworld_UnderfloorIsDirtNotGrass)
{
    auto result = generateOverworld(42, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    const auto& chunk = *result->centerChunk;
    i32 grassUnderground = 0;
    i32 dirtUnderground = 0;

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const i32 surfaceY = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            if (surfaceY < mc::world::MIN_BUILD_HEIGHT + 5) {
                continue;
            }
            // 检查地表下 2~4 格（underFloor 区域）
            for (i32 dy = 2; dy <= 4; ++dy) {
                const BlockState* block = chunk.getBlockState(x, surfaceY - dy, z);
                if (isBlock(block, VanillaBlocks::GRASS_BLOCK)) {
                    ++grassUnderground;
                } else if (isBlock(block, VanillaBlocks::DIRT)) {
                    ++dirtUnderground;
                }
            }
        }
    }

    // 地下泥土应远多于草方块（草方块不应出现在地下层）
    EXPECT_GT(dirtUnderground, grassUnderground)
        << "Underground should be mostly dirt, not grass. dirt=" << dirtUnderground << " grass=" << grassUnderground;
}

// ============================================================================
// 多种子一致性测试
// ============================================================================

/**
 * @brief 多种露出水面的陆地种子都应生成草方块
 *
 * 仅对露出水面的陆地生物群系列校验草方块。需排除两类本就不该有草方块的列：
 *   1. 海洋生物群系：海床不应用默认 onFloor 顶层规则（原版被 waterBlockCheck(-1,0)
 *      包裹，方块上方是水时跳过草/泥土），保持 STONE/GRAVEL。
 *   2. 被水淹没的陆地区域：地形低于海平面的陆地列被水填充，WorldSurfaceWG 高度图
 *      返回水面（水非空气）而非海床；水面方块显然不是草。
 * 某些种子在区块 (0,0) 的 3x3 区域内既无露出水面的陆地列（全海洋或全水淹海岸），
 * 此时没有可校验列，跳过该种子的断言（属预期，非 bug）。
 */
TEST_F(SurfaceRuleParityTest, Overworld_GrassBlockMultiSeed)
{
    constexpr u64 seeds[] = {42, 12345, 987654321, 0xCAFEBABE, 0xDEADBEEF};

    for (u64 seed : seeds) {
        auto result = generateOverworld(seed, 0, 0);
        ASSERT_NE(result->centerChunk, nullptr);

        const auto& chunk = *result->centerChunk;
        i32 grassCount = 0;
        i32 landColumns = 0;

        // 每隔 2 格采样
        for (i32 x = 0; x < 16; x += 2) {
            for (i32 z = 0; z < 16; z += 2) {
                const i32 surfaceY = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
                if (surfaceY < mc::world::MIN_BUILD_HEIGHT) {
                    continue;
                }
                const BlockState* block = chunk.getBlockState(x, surfaceY, z);
                // 海洋海床、或被水淹没列的水面方块，都正确地无草方块。
                if (block == nullptr || block->isLiquid() || _isOceanBiome(chunk.getBiomeAtBlock(x, surfaceY, z))) {
                    continue;
                }
                ++landColumns;
                if (isBlock(block, VanillaBlocks::GRASS_BLOCK)) {
                    ++grassCount;
                }
            }
        }

        // 整个区域都没有露出水面的陆地列可校验，跳过（属预期，非 bug）。
        if (landColumns == 0) {
            std::cout << "[GrassBlockMultiSeed] seed " << seed << " 区域无露出水面的陆地列，跳过草方块断言"
                      << std::endl;
            continue;
        }

        EXPECT_GT(grassCount, 0) << "Seed " << seed << " should produce grass blocks (landColumns=" << landColumns
                                 << "), found " << grassCount;
    }
}

// ============================================================================
// 恶地生物群系测试
// ============================================================================

/**
 * @brief 恶地表面应有陶土/红沙
 *
 * 验证恶地家族生物群系（Badlands, ErodedBadlands, WoodedBadlands）
 * 的表面方块包含恶地特有的方块类型。
 *
 * 种子/区块定位：经预扫描确认 seed=100 在 chunk(-6,-6) 整块为 WoodedBadlands（256 列全部命中恶地，
 * 215 列表面为陶土）。直接在此生成即可确定性命中恶地，无需大范围暴力扫描。
 * 原 r=5 暴力扫描（seed=12345）在该种子出生点附近不生成任何恶地，必然 GTEST_SKIP，
 * 从未真正校验过恶地表面规则；同时 121 个 3x3 生成严重超时。改为定向单次生成既快又必定命中。
 */
TEST_F(SurfaceRuleParityTest, Overworld_BadlandsHasTerracotta)
{
    bool foundBadlandsSurface = false;
    // 定向生成 seed=100 chunk(-6,-6)（已确认整块为恶地）。
    auto result = generateOverworld(100, -6, -6);
    ASSERT_NE(result->centerChunk, nullptr);

    const auto& chunk = *result->centerChunk;

    for (i32 x = 0; x < 16 && !foundBadlandsSurface; ++x) {
        for (i32 z = 0; z < 16 && !foundBadlandsSurface; ++z) {
            const i32 surfaceY = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            if (surfaceY < mc::world::MIN_BUILD_HEIGHT) {
                continue;
            }

            const BiomeId biome = chunk.getBiomeAtBlock(x, surfaceY, z);
            if (biome != Biomes::Badlands && biome != Biomes::ErodedBadlands && biome != Biomes::WoodedBadlands) {
                continue;
            }

            const BlockState* surfaceBlock = chunk.getBlockState(x, surfaceY, z);
            // 恶地表面应有这些方块之一
            if (isBlock(surfaceBlock, VanillaBlocks::ORANGE_TERRACOTTA) ||
                isBlock(surfaceBlock, VanillaBlocks::TERRACOTTA) || isBlock(surfaceBlock, VanillaBlocks::RED_SAND) ||
                isBlock(surfaceBlock, VanillaBlocks::WHITE_TERRACOTTA) ||
                isBlock(surfaceBlock, VanillaBlocks::COARSE_DIRT) ||
                isBlock(surfaceBlock, VanillaBlocks::GRASS_BLOCK) || isBlock(surfaceBlock, VanillaBlocks::DIRT)) {
                foundBadlandsSurface = true;
            }
        }
    }

    // 定向生成必定命中恶地；若未命中说明恶地生物群系生成或表面规则出现回归。
    EXPECT_TRUE(foundBadlandsSurface) << "seed=100 chunk(-6,-6) 应为恶地，但未找到恶地表面方块";
}

// ============================================================================
// 沙漠生物群系测试
// ============================================================================

/**
 * @brief 沙漠表面应为沙子
 *
 * 种子/区块定位：经预扫描确认 seed=8 在 chunk(2,-6) 含 48 列 Desert 生物群系，表面全部为 SAND
 * （y=64 平坦沙层）。直接在此生成即可确定性命中沙漠，无需大范围暴力扫描。
 * 原 r=8 暴力扫描（seed=98765）在该种子出生点 r=8 内不生成任何沙漠，必然 GTEST_SKIP，
 * 从未真正校验过沙漠表面规则；同时 289 个 3x3 生成（2601 次区块生成）严重超时（>300s CTest 上限）。
 * 改为定向单次生成既快又必定命中，让沙漠表面规则被真正断言而非静默跳过。
 */
TEST_F(SurfaceRuleParityTest, Overworld_DesertHasSand)
{
    bool foundDesert = false;
    bool desertHasSand = false;

    // 定向生成 seed=8 chunk(2,-6)（已确认含 48 列沙漠，表面全为沙子）。
    auto result = generateOverworld(8, 2, -6);
    ASSERT_NE(result->centerChunk, nullptr);

    const auto& chunk = *result->centerChunk;

    for (i32 x = 0; x < 16 && !foundDesert; ++x) {
        for (i32 z = 0; z < 16 && !foundDesert; ++z) {
            const i32 surfaceY = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            if (surfaceY < mc::world::MIN_BUILD_HEIGHT) {
                continue;
            }

            const BiomeId biome = chunk.getBiomeAtBlock(x, surfaceY, z);
            if (biome != Biomes::Desert) {
                continue;
            }

            foundDesert = true;
            const BlockState* surfaceBlock = chunk.getBlockState(x, surfaceY, z);
            desertHasSand =
                isBlock(surfaceBlock, VanillaBlocks::SAND) || isBlock(surfaceBlock, VanillaBlocks::SANDSTONE);
        }
    }

    // 定向生成必定命中沙漠；若未命中说明沙漠生物群系生成或表面规则出现回归。
    ASSERT_TRUE(foundDesert) << "seed=8 chunk(2,-6) 应含沙漠列，但未找到 Desert 生物群系";
    EXPECT_TRUE(desertHasSand) << "沙漠表面应为沙子或砂岩";
}

// ============================================================================
// 下界表面规则测试
// ============================================================================

/**
 * @brief 下界应生成下界岩
 */
TEST_F(SurfaceRuleParityTest, Nether_HasNetherrack)
{
    auto result = generateNether(42, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    const auto& chunk = *result->centerChunk;
    i32 netherrackCount = 0;

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            for (i32 y = 1; y < 127; ++y) {
                if (isBlock(chunk.getBlockState(x, y, z), VanillaBlocks::NETHERRACK)) {
                    ++netherrackCount;
                    break; // 每列只计数一次
                }
            }
        }
    }

    EXPECT_GT(netherrackCount, 0) << "Nether should have netherrack blocks";
}

/**
 * @brief 下界底部应有基岩层
 */
TEST_F(SurfaceRuleParityTest, Nether_BedrockAtBottom)
{
    auto result = generateNether(42, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    const auto& chunk = *result->centerChunk;
    i32 bedrockCount = 0;

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            if (isBlock(chunk.getBlockState(x, 0, z), VanillaBlocks::BEDROCK)) {
                ++bedrockCount;
            }
        }
    }

    EXPECT_GT(bedrockCount, 0) << "Nether bottom layer should have bedrock";
}

// ============================================================================
// 表面方块不应出现在错误层位的测试
// ============================================================================

/**
 * @brief 草方块不应出现在地下深处
 *
 * 草方块只应通过 onFloor + waterBlockCheck 规则出现在地表，
 * 不应通过错误的 underFloor 规则出现在地下层。
 */
TEST_F(SurfaceRuleParityTest, Overworld_GrassBlockNotDeepUnderground)
{
    auto result = generateOverworld(42, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    const auto& chunk = *result->centerChunk;
    i32 grassDeepUnderground = 0;

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const i32 surfaceY = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            if (surfaceY < mc::world::MIN_BUILD_HEIGHT + 10) {
                continue;
            }
            // 检查地表下 5~10 格深处
            for (i32 dy = 5; dy <= 10; ++dy) {
                if (surfaceY - dy < mc::world::MIN_BUILD_HEIGHT) {
                    break;
                }
                if (isBlock(chunk.getBlockState(x, surfaceY - dy, z), VanillaBlocks::GRASS_BLOCK)) {
                    ++grassDeepUnderground;
                }
            }
        }
    }

    // 深层不应有草方块
    EXPECT_EQ(grassDeepUnderground, 0) << "Grass blocks should not appear deep underground (5-10 blocks below surface)";
}

/**
 * @brief 地表附近不应有深板岩
 *
 * 表面规则应在 abovePreliminarySurface 为真时优先生效，
 * 深板岩 verticalGradient 在 abovePreliminarySurface 之后，不应覆盖表面方块。
 * 此测试验证深板岩不侵入到地表附近。
 */
TEST_F(SurfaceRuleParityTest, Overworld_NoDeepslateNearSurface)
{
    auto result = generateOverworld(42, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    const auto& chunk = *result->centerChunk;
    i32 deepslateNearSurface = 0;

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const i32 surfaceY = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            if (surfaceY < mc::world::MIN_BUILD_HEIGHT + 5) {
                continue;
            }
            // 检查地表及以上1格
            for (i32 dy = 0; dy <= 1; ++dy) {
                if (isBlock(chunk.getBlockState(x, surfaceY - dy, z), VanillaBlocks::DEEPSLATE)) {
                    ++deepslateNearSurface;
                }
            }
        }
    }

    EXPECT_EQ(deepslateNearSurface, 0) << "Deepslate should not appear at or near surface level";
}

// ============================================================================
// 多区块大面积测试
// ============================================================================

/**
 * @brief 地表方块类型包含草方块和泥土
 *
 * 验证地表和浅层地下具有合理的方块类型分布：
 * 草方块应在地表，泥土应在地下层，石头应在更深处。
 */
TEST_F(SurfaceRuleParityTest, Overworld_SurfaceBlockDiversity)
{
    auto result = generateOverworld(42, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    bool foundGrass = false;
    bool foundDirt = false;
    bool foundStone = false;

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const i32 surfaceY = result->centerChunk->getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            if (surfaceY < mc::world::MIN_BUILD_HEIGHT) {
                continue;
            }
            const BlockState* surfaceBlock = result->centerChunk->getBlockState(x, surfaceY, z);
            if (isBlock(surfaceBlock, VanillaBlocks::GRASS_BLOCK)) {
                foundGrass = true;
            }
            // 检查地下层（地表下2~8格范围）
            for (i32 dy = 2; dy <= 8 && surfaceY - dy >= mc::world::MIN_BUILD_HEIGHT; ++dy) {
                const BlockState* below = result->centerChunk->getBlockState(x, surfaceY - dy, z);
                if (isBlock(below, VanillaBlocks::DIRT)) {
                    foundDirt = true;
                }
                if (isBlock(below, VanillaBlocks::STONE)) {
                    foundStone = true;
                }
            }
        }
    }

    EXPECT_TRUE(foundGrass) << "Overworld surface should have grass blocks";
    EXPECT_TRUE(foundDirt) << "Overworld underground should have dirt";
    EXPECT_TRUE(foundStone) << "Overworld deep underground should have stone";
}

/**
 * @brief 表面方块不应包含深板岩
 *
 * 无论哪个生物群系，地表方块都不应该是深板岩，
 * 因为深板岩规则在 abovePreliminarySurface 之后，
 * 不会覆盖表面规则的匹配结果。
 */
TEST_F(SurfaceRuleParityTest, Overworld_NoDeepslateOnSurface)
{
    auto result = generateOverworld(42, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    const BlockState* deepslate = &block_registry::DeepslateBlocks::DEEPSLATE->defaultState();
    i32 deepslateOnSurface = 0;

    for (auto& chunk : result->ownedChunks) {
        for (i32 x = 0; x < 16; ++x) {
            for (i32 z = 0; z < 16; ++z) {
                const i32 surfaceY = chunk->getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
                if (surfaceY < mc::world::MIN_BUILD_HEIGHT) {
                    continue;
                }
                const BlockState* block = chunk->getBlockState(x, surfaceY, z);
                if (block == deepslate) {
                    ++deepslateOnSurface;
                }
            }
        }
    }

    EXPECT_EQ(deepslateOnSurface, 0) << "Deepslate should not appear as surface block";
}

} // namespace
} // namespace mc
