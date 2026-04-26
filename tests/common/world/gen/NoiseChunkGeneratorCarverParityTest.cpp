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
                    chunk.setBlock(x, y, z, stone);
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
                    const BlockState* state = chunk.getBlock(x, y, z);
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
                    const BlockState* l = lhs.getBlock(x, y, z);
                    const BlockState* r = rhs.getBlock(x, y, z);
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

} // namespace
} // namespace mc
