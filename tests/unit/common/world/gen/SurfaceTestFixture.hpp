#pragma once

#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/source/FixedBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/world/gen/RandomState.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace mc::test {

/**
 * @brief 固定生物群系和露出水面的石质平台，保留真实噪声上下文与地表规则执行入口
 */
struct SurfaceTestFixture {
    std::vector<std::unique_ptr<world::chunk::ChunkPrimer>> chunks;
    std::unique_ptr<WorldGenRegion> region;
    std::unique_ptr<NoiseChunkGenerator> generator;
    world::chunk::ChunkPrimer* center;
};

/**
 * @brief 构造目标生物群系的地表规则输入，避免测试依赖某种子偶然出现的气候和海岸位置
 */
inline std::unique_ptr<SurfaceTestFixture> makeSurfaceTestFixture(u64 seed, BiomeId biome)
{
    auto result = std::make_unique<SurfaceTestFixture>();
    auto settings = DimensionSettings::overworld();
    auto randomState = world::gen::RandomState::create(settings, seed);
    auto biomeSource = std::make_unique<world::biome::source::FixedBiomeSource>(seed, biome);
    result->generator =
        std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));

    std::vector<world::chunk::IChunk*> pointers;
    for (i32 z = -1; z <= 1; ++z) {
        for (i32 x = -1; x <= 1; ++x) {
            auto chunk = std::make_unique<world::chunk::ChunkPrimer>(x, z);
            pointers.push_back(chunk.get());
            result->chunks.push_back(std::move(chunk));
        }
    }
    result->center = result->chunks[4].get();
    result->region = std::make_unique<WorldGenRegion>(0, 0, 1, std::move(pointers), 0);
    for (auto& chunk : result->chunks) {
        result->generator->generateBiomes(*result->region, *chunk);
    }
    // 真实生成阶段建立 NoiseChunk，供 abovePreliminarySurface 条件查询预备地表高度。
    auto& center = *result->center;
    result->generator->generateNoise(*result->region, center);
    constexpr i32 SURFACE_Y = world::SEA_LEVEL + 64;
    for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
        for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
            for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MAX_BUILD_HEIGHT; ++y) {
                center.setBlockState(x,
                    y,
                    z,
                    y <= SURFACE_Y ? &VanillaBlocks::STONE->defaultState() : &VanillaBlocks::AIR->defaultState());
            }
        }
    }
    result->generator->buildSurface(*result->region, center);
    return result;
}

} // namespace mc::test
