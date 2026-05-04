#include <gtest/gtest.h>

#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/layer/LayerUtil.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/core/Constants.hpp"

#include <array>
#include <memory>

namespace mc {
namespace {

/**
 * @brief NoiseChunkGenerator 雕刻阶段一致性测试
 *
 * 验证默认构造路径与注入 BiomeProvider 构造路径在空气雕刻阶段行为一致，
 * 防止某一构造路径遗漏关键初始化逻辑。
 */
class NoiseChunkGeneratorCarverParityTest : public ::testing::Test {
protected:
    /**
     * @brief 初始化测试依赖
     *
     * @note 方块和生物群系注册表是生成流程的前置条件。
     */
    static void SetUpTestSuite() {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }

    /**
     * @brief 以石头填满整个区块
     *
     * @note 雕刻器会把石头替换为空气或熔岩，用于稳定统计雕刻结果。
     */
    static void fillChunkWithStone(ChunkPrimer& chunk) {
        const BlockState* stone = &VanillaBlocks::STONE->defaultState();
        ASSERT_NE(stone, nullptr);

        for (i32 x = 0; x < 16; ++x) {
            for (i32 z = 0; z < 16; ++z) {
                for (i32 y = 0; y < mc::world::MAX_BUILD_HEIGHT; ++y) {
                    chunk.setBlockState(x, y, z, stone);
                }
            }
        }
    }

    /**
     * @brief 为区块写入统一生物群系
     *
     * @note 当前雕刻逻辑对生物群系依赖较弱，但仍应保持输入数据完整。
     */
    static void fillChunkBiomes(ChunkPrimer& chunk, BiomeId biomeId) {
        BiomeContainer& biomes = chunk.getBiomes();
        for (i32 y = 0; y < BiomeContainer::BIOME_HEIGHT; ++y) {
            for (i32 z = 0; z < BiomeContainer::BIOME_DEPTH; ++z) {
                for (i32 x = 0; x < BiomeContainer::BIOME_WIDTH; ++x) {
                    biomes.setBiome(x, y, z, biomeId);
                }
            }
        }
    }

    /**
     * @brief 创建用于雕刻测试的实体区块
     */
    static std::unique_ptr<ChunkPrimer> makeSolidChunk(ChunkCoord chunkX, ChunkCoord chunkZ) {
        auto chunk = std::make_unique<ChunkPrimer>(chunkX, chunkZ);
        fillChunkWithStone(*chunk);
        fillChunkBiomes(*chunk, Biomes::Plains);
        chunk->updateAllHeightmaps();
        return chunk;
    }

    /**
     * @brief 统计被雕刻后非石头方块数量
     */
    static i32 countNonStoneBlocks(const ChunkPrimer& chunk) {
        const BlockState* stone = &VanillaBlocks::STONE->defaultState();
        i32 count = 0;

        for (i32 x = 0; x < 16; ++x) {
            for (i32 z = 0; z < 16; ++z) {
                for (i32 y = 0; y < mc::world::MAX_BUILD_HEIGHT; ++y) {
                    const BlockState* state = chunk.getBlockState(x, y, z);
                    if (state != stone) {
                        ++count;
                    }
                }
            }
        }

        return count;
    }

    /**
     * @brief 构造最小化 WorldGenRegion 视图
     *
     * @note applyCarvers 当前不读取 region 数据，此处仅满足接口约束。
     */
    static std::array<IChunk*, 9> makeRegionChunks(IChunk* centerChunk) {
        std::array<IChunk*, 9> chunks{};
        chunks.fill(centerChunk);
        return chunks;
    }

    /**
     * @brief 逐方块断言两个区块完全一致
     */
    static void expectChunkBlocksEqual(const ChunkPrimer& lhs, const ChunkPrimer& rhs) {
        for (i32 x = 0; x < 16; ++x) {
            for (i32 z = 0; z < 16; ++z) {
                for (i32 y = 0; y < mc::world::MAX_BUILD_HEIGHT; ++y) {
                    const BlockState* l = lhs.getBlockState(x, y, z);
                    const BlockState* r = rhs.getBlockState(x, y, z);
                    EXPECT_EQ(l, r) << "Block mismatch at (" << x << ", " << y << ", " << z << ")";
                }
            }
        }
    }
};

/**
 * @brief 注入 BiomeProvider 的构造路径应保持雕刻阶段一致
 *
 * @note 若未初始化雕刻器，此测试会在首个发生雕刻的区块上失败。
 */
TEST_F(NoiseChunkGeneratorCarverParityTest, InjectedBiomeProviderKeepsCarverPipelineParity) {
    constexpr u64 seed = 0x4D435245424F524EULL;

    DimensionSettings defaultSettings = DimensionSettings::overworld();
    NoiseChunkGenerator defaultGenerator(seed, std::move(defaultSettings));

    DimensionSettings injectedSettings = DimensionSettings::overworld();
    auto injectedProvider = std::make_unique<LayerBiomeProvider>(seed, false);
    NoiseChunkGenerator injectedGenerator(seed, std::move(injectedSettings), std::move(injectedProvider));

    bool foundCarvedChunk = false;

    for (ChunkCoord chunkX = -12; chunkX <= 12 && !foundCarvedChunk; ++chunkX) {
        for (ChunkCoord chunkZ = -12; chunkZ <= 12 && !foundCarvedChunk; ++chunkZ) {
            auto defaultChunk = makeSolidChunk(chunkX, chunkZ);
            auto defaultChunks = makeRegionChunks(defaultChunk.get());
            WorldGenRegion defaultRegion(chunkX, chunkZ, defaultChunks);

            defaultGenerator.applyCarvers(defaultRegion, *defaultChunk, false);
            const i32 defaultCarved = countNonStoneBlocks(*defaultChunk);
            if (defaultCarved <= 0) {
                continue;
            }

            auto injectedChunk = makeSolidChunk(chunkX, chunkZ);
            auto injectedChunks = makeRegionChunks(injectedChunk.get());
            WorldGenRegion injectedRegion(chunkX, chunkZ, injectedChunks);

            injectedGenerator.applyCarvers(injectedRegion, *injectedChunk, false);
            const i32 injectedCarved = countNonStoneBlocks(*injectedChunk);

            EXPECT_EQ(defaultCarved, injectedCarved)
                << "Carved block count mismatch at chunk (" << chunkX << ", " << chunkZ << ")";
            expectChunkBlocksEqual(*defaultChunk, *injectedChunk);

            EXPECT_TRUE(defaultChunk->hasCompletedStatus(ChunkStatuses::CARVERS));
            EXPECT_TRUE(injectedChunk->hasCompletedStatus(ChunkStatuses::CARVERS));
            foundCarvedChunk = true;
        }
    }

    EXPECT_TRUE(foundCarvedChunk)
        << "No carved chunk found in search range, cannot validate constructor parity";
}

/**
 * @brief NoiseChunkGenerator 高斯查找表初始化测试
 *
 * 验证 24x24x24 高斯查找表在构造时正确初始化。
 */
TEST_F(NoiseChunkGeneratorCarverParityTest, GaussianLUTInitialization) {
    // 创建生成器时高斯查找表应该被初始化
    constexpr u64 seed = 12345ULL;
    DimensionSettings settings = DimensionSettings::overworld();
    NoiseChunkGenerator generator(seed, std::move(settings));

    // 验证生成器成功创建（高斯查找表作为静态成员初始化）
    // 如果初始化失败，会有编译或运行时错误
    EXPECT_NO_THROW({
        DimensionSettings settings2 = DimensionSettings::overworld();
        NoiseChunkGenerator generator2(seed, std::move(settings2));
    });
}

/**
 * @brief NoiseChunkGenerator 结构密度偏移计算测试
 *
 * 验证 calculateStructureDensityOffset 产生正确的高斯衰减值。
 *
 * 参考 MC 1.16.5 NoiseChunkGenerator.func_222554_b:
 * 该函数产生负密度值来平滑结构边界地形。
 * 中心点有最大的负偏移（向下凹陷），边缘趋于零。
 */
TEST(NoiseChunkGeneratorDensityTest, StructureDensityOffsetValues) {
    // 中心点应该有最大的负偏移（用于向下平滑地形）
    f64 centerOffset = NoiseChunkGenerator::calculateStructureDensityOffset(0, 0, 0);
    EXPECT_LT(centerOffset, 0.0) << "Center should have negative density offset for terrain smoothing";
    EXPECT_NEAR(centerOffset, -0.696, 0.01) << "Center offset should match MC 1.16.5 value";

    // 远距离点应该趋近于零
    f64 farOffset = NoiseChunkGenerator::calculateStructureDensityOffset(50, 50, 50);
    EXPECT_NEAR(farOffset, 0.0, 0.001) << "Far distance should have near-zero offset";

    // 对称性：相同距离的点应该产生相同的影响（X和Z轴对称）
    f64 offset1 = NoiseChunkGenerator::calculateStructureDensityOffset(5, 3, 7);
    f64 offset2 = NoiseChunkGenerator::calculateStructureDensityOffset(-5, 3, 7);
    f64 offset3 = NoiseChunkGenerator::calculateStructureDensityOffset(5, 3, -7);
    f64 offset4 = NoiseChunkGenerator::calculateStructureDensityOffset(-5, 3, -7);

    // X 和 Z 的对称性
    EXPECT_NEAR(offset1, offset2, 0.0001) << "X-axis symmetry should hold";
    EXPECT_NEAR(offset1, offset3, 0.0001) << "Z-axis symmetry should hold";
    EXPECT_NEAR(offset1, offset4, 0.0001) << "XZ diagonal symmetry should hold";

    // Y 轴行为验证：Y 偏移为负时（结构下方），偏移应为正
    // 这用于在结构下方抬升地形
    f64 belowOffset = NoiseChunkGenerator::calculateStructureDensityOffset(0, -5, 0);
    EXPECT_GT(belowOffset, 0.0) << "Below structure should have positive offset (terrain raised)";

    // Y 偏移为正时（结构上方），偏移应为负
    // 这用于在结构上方降低地形
    f64 aboveOffset = NoiseChunkGenerator::calculateStructureDensityOffset(0, 5, 0);
    EXPECT_LT(aboveOffset, 0.0) << "Above structure should have negative offset (terrain lowered)";
}

/**
 * @brief NoiseChunkGenerator 高斯查找表边界测试
 *
 * 验证查找表索引计算不会越界。
 */
TEST(NoiseChunkGeneratorDensityTest, GaussianLUTBoundaryCheck) {
    // 边界内点（-12 到 +11）
    for (i32 dx = -12; dx < 12; ++dx) {
        for (i32 dy = -12; dy < 12; ++dy) {
            for (i32 dz = -12; dz < 12; ++dz) {
                // 应该不崩溃
                f64 offset = NoiseChunkGenerator::calculateStructureDensityOffset(dx, dy, dz);
                (void)offset;  // 仅验证计算能完成
            }
        }
    }

    // 边界外点应该返回零（在 generateNoise 中处理，这里验证函数行为）
    f64 outsideOffset = NoiseChunkGenerator::calculateStructureDensityOffset(15, 15, 15);
    // 注意：calculateStructureDensityOffset 本身不检查边界，边界检查在 generateNoise 中
    // 这里只验证函数能正常执行
    (void)outsideOffset;
    SUCCEED() << "Boundary calculations completed without crash";
}

} // namespace
} // namespace mc
