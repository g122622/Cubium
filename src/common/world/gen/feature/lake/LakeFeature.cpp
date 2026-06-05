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

#include "LakeFeature.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <mutex>

namespace {

std::mutex g_lakeFeaturesMutex;

/// 湖泊布尔数组尺寸常量（参考 MC 1.21.11: LakeFeature）
constexpr i32 LAKE_SIZE_X = 16;
constexpr i32 LAKE_SIZE_Y = 8;
constexpr i32 LAKE_SIZE_Z = 16;
constexpr i32 LAKE_ARRAY_SIZE = LAKE_SIZE_X * LAKE_SIZE_Z * LAKE_SIZE_Y; // 2048

/// 流体表面高度（在布尔数组 Y 轴上的分界线，0-3 为流体，4-7 为空气）
constexpr i32 FLUID_SURFACE_Y = 4;

/// 索引计算：(x * 16 + z) * 8 + y
[[nodiscard]] inline i32 lakeIndex(i32 x, i32 z, i32 y)
{
    return (x * LAKE_SIZE_Z + z) * LAKE_SIZE_Y + y;
}

} // namespace

namespace mc::world::gen::feature::lake {

LakeFeature::LakeFeature(const LakeFeatureConfig& config)
    : m_config(config)
{}

bool LakeFeature::place(WorldGenRegion& world, math::Random& rng, i32 x, i32 y, i32 z)
{
    if (m_config.fluidState == nullptr) {
        return false;
    }

    // MC 1.21.11: 如果 Y 低于 minY + 4，则无法生成
    // 原版检查 blockpos.getY() <= world.getMinBuildHeight() + 4
    if (y <= world::MIN_BUILD_HEIGHT + 4) {
        return false;
    }

    // 中心位置下移 4 格
    y -= 4;

    // 1. 初始化布尔数组
    std::array<bool, LAKE_ARRAY_SIZE> lakeMap{};
    lakeMap.fill(false);

    // 2. 生成 4~7 个随机椭球体
    const i32 numEllipsoids = 4 + rng.nextInt(4);
    for (i32 e = 0; e < numEllipsoids; ++e) {
        // 随机半径
        const f64 rx = rng.nextDouble() * 6.0 + 3.0; // 3.0 ~ 9.0
        const f64 ry = rng.nextDouble() * 4.0 + 2.0; // 2.0 ~ 6.0
        const f64 rz = rng.nextDouble() * 6.0 + 3.0; // 3.0 ~ 9.0

        // 随机中心（确保椭球在数组范围内）
        const f64 cx = rng.nextDouble() * (16.0 - rx - 2.0) + 1.0 + rx / 2.0;
        const f64 cy = rng.nextDouble() * (8.0 - ry - 4.0) + 2.0 + ry / 2.0;
        const f64 cz = rng.nextDouble() * (16.0 - rz - 2.0) + 1.0 + rz / 2.0;

        // 遍历内部区域（边缘不参与，避免边界问题）
        for (i32 bx = 1; bx < LAKE_SIZE_X - 1; ++bx) {
            const f64 dx = (static_cast<f64>(bx) - cx) / (rx / 2.0);
            // 提前退出：X 方向已经超出椭球
            if (dx * dx >= 1.0) continue;

            for (i32 bz = 1; bz < LAKE_SIZE_Z - 1; ++bz) {
                const f64 dz = (static_cast<f64>(bz) - cz) / (rz / 2.0);
                // 提前退出：X+Z 方向已经超出
                if (dx * dx + dz * dz >= 1.0) continue;

                for (i32 by = 1; by < LAKE_SIZE_Y - 1; ++by) {
                    const f64 dy = (static_cast<f64>(by) - cy) / (ry / 2.0);
                    if (dx * dx + dy * dy + dz * dz < 1.0) {
                        lakeMap[lakeIndex(bx, bz, by)] = true;
                    }
                }
            }
        }
    }

    // 3. 验证边界（检查是否与不兼容的方块冲突）
    for (i32 bx = 0; bx < LAKE_SIZE_X; ++bx) {
        for (i32 bz = 0; bz < LAKE_SIZE_Z; ++bz) {
            for (i32 by = 0; by < LAKE_SIZE_Y; ++by) {
                const bool isInside = lakeMap[lakeIndex(bx, bz, by)];

                // 检查是否为边界格（自身不在湖内，但有相邻格在湖内）
                if (!isInside &&
                    ((bx > 0 && lakeMap[lakeIndex(bx - 1, bz, by)]) ||
                        (bx < LAKE_SIZE_X - 1 && lakeMap[lakeIndex(bx + 1, bz, by)]) ||
                        (bz > 0 && lakeMap[lakeIndex(bx, bz - 1, by)]) ||
                        (bz < LAKE_SIZE_Z - 1 && lakeMap[lakeIndex(bx, bz + 1, by)]) ||
                        (by > 0 && lakeMap[lakeIndex(bx, bz, by - 1)]) ||
                        (by < LAKE_SIZE_Y - 1 && lakeMap[lakeIndex(bx, bz, by + 1)]))) {
                    const BlockState* state = world.getBlockState(x + bx, y + by, z + bz);
                    if (!state) continue;

                    // Y >= 4 的边界格如果碰到液体，则不能生成
                    if (by >= FLUID_SURFACE_Y && state->isLiquid()) {
                        return false;
                    }

                    // Y < 4 的边界格如果不是固体且不是同种流体，则不能生成
                    if (by < FLUID_SURFACE_Y) {
                        if (!state->isSolid() && state->getBlock() != m_config.fluidBlock) {
                            return false;
                        }
                    }
                }
            }
        }
    }

    // 4. 雕刻湖泊内部
    const BlockState* caveAir = VanillaBlocks::getState(VanillaBlocks::CAVE_AIR);
    if (!caveAir) {
        caveAir = VanillaBlocks::getState(VanillaBlocks::AIR);
    }

    for (i32 bx = 0; bx < LAKE_SIZE_X; ++bx) {
        for (i32 bz = 0; bz < LAKE_SIZE_Z; ++bz) {
            for (i32 by = 0; by < LAKE_SIZE_Y; ++by) {
                if (!lakeMap[lakeIndex(bx, bz, by)]) {
                    continue;
                }

                const BlockState* state = world.getBlockState(x + bx, y + by, z + bz);
                if (!state || !canReplaceBlock(*state)) {
                    continue;
                }

                if (by >= FLUID_SURFACE_Y) {
                    // Y >= 4：填充洞穴空气
                    world.setBlockState(x + bx, y + by, z + bz, caveAir);
                } else {
                    // Y < 4：填充流体
                    world.setBlockState(x + bx, y + by, z + bz, m_config.fluidState);
                }
            }
        }
    }

    // 5. 放置边界方块
    if (m_config.borderState && m_config.borderBlock && !m_config.borderBlock->defaultState().isAir()) {
        for (i32 bx = 0; bx < LAKE_SIZE_X; ++bx) {
            for (i32 bz = 0; bz < LAKE_SIZE_Z; ++bz) {
                for (i32 by = 0; by < LAKE_SIZE_Y; ++by) {
                    const bool isInside = lakeMap[lakeIndex(bx, bz, by)];

                    // 边界格：自身不在湖内但有相邻格在湖内
                    if (!isInside &&
                        ((bx > 0 && lakeMap[lakeIndex(bx - 1, bz, by)]) ||
                            (bx < LAKE_SIZE_X - 1 && lakeMap[lakeIndex(bx + 1, bz, by)]) ||
                            (bz > 0 && lakeMap[lakeIndex(bx, bz - 1, by)]) ||
                            (bz < LAKE_SIZE_Z - 1 && lakeMap[lakeIndex(bx, bz + 1, by)]) ||
                            (by > 0 && lakeMap[lakeIndex(bx, bz, by - 1)]) ||
                            (by < LAKE_SIZE_Y - 1 && lakeMap[lakeIndex(bx, bz, by + 1)]))) {
                        const BlockState* state = world.getBlockState(x + bx, y + by, z + bz);
                        if (!state || !state->isSolid()) {
                            continue;
                        }

                        // Y >= 4 的边界有 50% 概率放置边界方块
                        // Y < 4 的边界总是放置
                        if (by >= FLUID_SURFACE_Y && rng.nextInt(2) != 0) {
                            continue;
                        }

                        world.setBlockState(x + bx, y + by, z + bz, m_config.borderState);
                    }
                }
            }
        }
    }

    // 6. 水湖冻结检查（Y = FLUID_SURFACE_Y 处的流体表面）
    if (m_config.fluidBlock == VanillaBlocks::WATER) {
        for (i32 bx = 0; bx < LAKE_SIZE_X; ++bx) {
            for (i32 bz = 0; bz < LAKE_SIZE_Z; ++bz) {
                const i32 surfaceY = y + FLUID_SURFACE_Y;
                const BlockState* state = world.getBlockState(x + bx, surfaceY, z + bz);

                if (state && state->getBlock() == VanillaBlocks::WATER) {
                    // 检查生物群系是否足够冷以冻结
                    auto biomeId = world.getBiome(x + bx, surfaceY, z + bz);
                    // 简化：通过高度判断冻结（MC 使用 Biome.shouldFreeze）
                    // TODO: 完整实现需要 Biome.shouldFreeze(world, pos)
                    // 当前使用 Biome 的温度方法判断
                    // 此处保守处理，先不冻结，待 Biome 集成完善后补充
                }
            }
        }
    }

    return true;
}

bool LakeFeature::canReplaceBlock(const BlockState& state)
{
    // MC 1.21.11: !state.is(BlockTags.FEATURES_CANNOT_REPLACE)
    // 当项目添加 FEATURES_CANNOT_REPLACE 标签后替换为标签检查
    // 目前使用简单判断：空气和流体可替换，其他方块需要进一步判断
    return state.isAir() || state.isLiquid() || (!state.isSolid() && state.getBlock() != VanillaBlocks::BEDROCK);
}

LakeFeatureConfig LakeFeature::createWaterLake()
{
    return LakeFeatureConfig(VanillaBlocks::WATER, VanillaBlocks::STONE);
}

LakeFeatureConfig LakeFeature::createLavaLake()
{
    return LakeFeatureConfig(VanillaBlocks::LAVA, VanillaBlocks::STONE);
}

std::unique_ptr<LakeFeature> createWaterLakeFeature()
{
    return std::make_unique<LakeFeature>(LakeFeature::createWaterLake());
}

std::unique_ptr<LakeFeature> createLavaLakeFeature()
{
    return std::make_unique<LakeFeature>(LakeFeature::createLavaLake());
}

} // namespace mc::world::gen::feature::lake

namespace mc {

std::vector<std::unique_ptr<ConfiguredLakeFeature>> LakeFeatures::s_features;

ConfiguredLakeFeature::ConfiguredLakeFeature(
    world::gen::feature::lake::LakeFeatureConfig config, const char* featureName, i32 chance, i32 minY, i32 maxY)
    : m_feature(config)
    , m_name(featureName)
    , m_chance(std::max(1, chance))
    , m_minY(minY)
    , m_maxY(std::max(minY, maxY))
    , m_isLava(config.fluidBlock == VanillaBlocks::LAVA)
{}

bool ConfiguredLakeFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    (void)chunk;

    if (m_chance > 1 && random.nextInt(m_chance) != 0) {
        return false;
    }

    i32 minY = m_minY;
    i32 maxY = m_maxY;

    if (m_isLava) {
        const i32 seaLevel = generator.seaLevel();
        constexpr i32 halfSectionHeight = world::CHUNK_SECTION_HEIGHT / 2;
        maxY = std::min(maxY, seaLevel + halfSectionHeight);

        // 熔岩湖通常更偏地下，保留少量高位湖泊
        if (random.nextInt(10) != 0) {
            maxY = std::min(maxY, seaLevel - halfSectionHeight);
        }
        maxY = std::max(maxY, minY);
    }

    const i32 x = pos.x + random.nextInt(world::CHUNK_WIDTH) + world::CHUNK_WIDTH / 2;
    const i32 z = pos.z + random.nextInt(world::CHUNK_WIDTH) + world::CHUNK_WIDTH / 2;
    const i32 y = minY + random.nextInt(maxY - minY + 1);

    return m_feature.place(region, random, x, y, z);
}

void LakeFeatures::initialize()
{
    std::lock_guard<std::mutex> lock(g_lakeFeaturesMutex);
    s_features.clear();
    s_features.push_back(createWaterLake());
    s_features.push_back(createLavaLake());
}

const std::vector<std::unique_ptr<ConfiguredLakeFeature>>& LakeFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredLakeFeature>> LakeFeatures::getAllFeaturesAndClear()
{
    std::lock_guard<std::mutex> lock(g_lakeFeaturesMutex);
    std::vector<std::unique_ptr<ConfiguredLakeFeature>> result;
    result.swap(s_features);
    return result;
}

std::unique_ptr<ConfiguredLakeFeature> LakeFeatures::createWaterLake()
{
    constexpr i32 WATER_LAKE_MAX_Y = world::MAX_BUILD_HEIGHT / 2 - 1;
    return std::make_unique<ConfiguredLakeFeature>(
        world::gen::feature::lake::LakeFeature::createWaterLake(), "water_lake", 4, 20, WATER_LAKE_MAX_Y);
}

std::unique_ptr<ConfiguredLakeFeature> LakeFeatures::createLavaLake()
{
    return std::make_unique<ConfiguredLakeFeature>(
        world::gen::feature::lake::LakeFeature::createLavaLake(), "lava_lake", 8, 10, world::SEA_LEVEL);
}

} // namespace mc
