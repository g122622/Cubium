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
#include "../../../../core/Constants.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorldWriter.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include <algorithm>
#include <cmath>
#include <mutex>

namespace {

std::mutex g_lakeFeaturesMutex;

} // namespace

namespace mc::world::gen::feature::lake {

LakeFeature::LakeFeature(const LakeFeatureConfig& config)
    : m_config(config)
{}

bool LakeFeature::place(IWorldWriter& world, math::Random& rng, i32 x, i32 y, i32 z)
{
    if (m_config.fluidState == nullptr) {
        return false;
    }

    // 湖泊大小参数
    constexpr i32 RADIUS_X = 8;
    constexpr i32 RADIUS_Y = 4;
    constexpr i32 RADIUS_Z = 8;

    // 检查是否适合生成
    if (!canPlaceAt(world, x, y, z)) {
        return false;
    }

    // 生成椭圆形湖泊
    for (i32 dx = -RADIUS_X; dx <= RADIUS_X; ++dx) {
        for (i32 dy = -RADIUS_Y; dy <= RADIUS_Y; ++dy) {
            for (i32 dz = -RADIUS_Z; dz <= RADIUS_Z; ++dz) {
                // 椭球方程
                f32 dist = static_cast<f32>(dx * dx) / static_cast<f32>(RADIUS_X * RADIUS_X) +
                    static_cast<f32>(dy * dy) / static_cast<f32>(RADIUS_Y * RADIUS_Y) +
                    static_cast<f32>(dz * dz) / static_cast<f32>(RADIUS_Z * RADIUS_Z);

                if (dist <= 1.0f) {
                    // 内部填充流体
                    world.setBlockState(x + dx, y + dy, z + dz, m_config.fluidState);
                } else if (dist <= 1.25f && dy <= 0) {
                    // 边界区域
                    if (m_config.borderState) {
                        world.setBlockState(x + dx, y + dy, z + dz, m_config.borderState);
                    }
                }
            }
        }
    }

    // 在底部添加一些随机方块（增加自然感）
    if (m_config.borderState) {
        for (i32 i = 0; i < 8; ++i) {
            i32 dx = rng.nextInt(RADIUS_X * 2) - RADIUS_X;
            i32 dz = rng.nextInt(RADIUS_Z * 2) - RADIUS_Z;
            i32 dy = -RADIUS_Y + rng.nextInt(2);

            f32 dist = static_cast<f32>(dx * dx) / static_cast<f32>(RADIUS_X * RADIUS_X) +
                static_cast<f32>(dy * dy) / static_cast<f32>(RADIUS_Y * RADIUS_Y) +
                static_cast<f32>(dz * dz) / static_cast<f32>(RADIUS_Z * RADIUS_Z);

            if (dist <= 1.25f) {
                world.setBlockState(x + dx, y + dy - 1, z + dz, m_config.borderState);
            }
        }
    }

    return true;
}

bool LakeFeature::canPlaceAt(IWorldWriter& world, i32 x, i32 y, i32 z) const
{
    // 参考 MC 1.16.5: 湖泊生成位置检查
    // 水湖限制: Y >= 8
    // 熔岩湖限制: Y >= 8，在较低高度更常见

    if (y < 8 || y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    // 尝试从 WorldGenRegion 读取周围方块，避免生成在大面积空腔中
    const auto* region = dynamic_cast<const WorldGenRegion*>(&world);
    if (!region) {
        return true;
    }

    i32 solidCount = 0;
    i32 sampleCount = 0;
    constexpr i32 CHECK_RADIUS = 2;

    for (i32 dx = -CHECK_RADIUS; dx <= CHECK_RADIUS; ++dx) {
        for (i32 dy = -CHECK_RADIUS; dy <= CHECK_RADIUS; ++dy) {
            for (i32 dz = -CHECK_RADIUS; dz <= CHECK_RADIUS; ++dz) {
                if (std::abs(dx) <= 1 && std::abs(dy) <= 1 && std::abs(dz) <= 1) {
                    continue;
                }

                ++sampleCount;
                const BlockState* state = region->getBlockState(x + dx, y + dy, z + dz);
                if (state && (state->isSolid() || state->isLiquid())) {
                    ++solidCount;
                }
            }
        }
    }

    // 熔岩湖对支撑要求更高，尽量避免悬空熔岩池
    const i32 thresholdMultiplier = (m_config.fluidBlock == VanillaBlocks::LAVA) ? 3 : 2;
    return sampleCount > 0 && solidCount * thresholdMultiplier >= sampleCount * 2;
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
        maxY = std::min(maxY, seaLevel + 8);

        // 熔岩湖通常更偏地下，保留少量高位湖泊。
        if (random.nextInt(10) != 0) {
            maxY = std::min(maxY, seaLevel - 8);
        }
        maxY = std::max(maxY, minY);
    }

    const i32 x = pos.x + random.nextInt(16) + 8;
    const i32 z = pos.z + random.nextInt(16) + 8;
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
    return std::make_unique<ConfiguredLakeFeature>(
        world::gen::feature::lake::LakeFeature::createWaterLake(), "water_lake", 4, 20, 127);
}

std::unique_ptr<ConfiguredLakeFeature> LakeFeatures::createLavaLake()
{
    return std::make_unique<ConfiguredLakeFeature>(
        world::gen::feature::lake::LakeFeature::createLavaLake(), "lava_lake", 8, 10, 63);
}

} // namespace mc
