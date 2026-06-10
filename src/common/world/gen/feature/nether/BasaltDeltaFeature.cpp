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

#include "BasaltDeltaFeature.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc {

// ============================================================================
// BasaltDeltaFeature 实现
// ============================================================================

bool BasaltDeltaFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const BasaltDeltaFeatureConfig& config)
{
    // 获取方块状态
    const BlockState* basalt = VanillaBlocks::getState(VanillaBlocks::BASALT);
    const BlockState* magma = VanillaBlocks::getState(VanillaBlocks::MAGMA);
    const BlockState* netherrack = VanillaBlocks::getState(VanillaBlocks::NETHERRACK);

    if (!basalt || !netherrack) {
        return false;
    }

    // 在区域内生成玄武岩地面
    i32 halfSize = config.size / 2;

    for (i32 dx = -halfSize; dx <= halfSize; ++dx) {
        for (i32 dz = -halfSize; dz <= halfSize; ++dz) {
            // 使用圆形掩码
            f32 distSq = static_cast<f32>(dx * dx + dz * dz);
            f32 radiusSq = static_cast<f32>(halfSize * halfSize);
            if (distSq > radiusSq) {
                continue;
            }

            // 边缘渐变
            f32 edgeFactor = 1.0f - (distSq / radiusSq);
            if (random.nextFloat() > edgeFactor) {
                continue;
            }

            BlockPos placePos(pos.x + dx, pos.y, pos.z + dz);

            // 检查当前位置
            const BlockState* currentState = world.getBlockState(placePos);
            if (!currentState || !currentState->is(VanillaBlocks::NETHERRACK)) {
                continue;
            }

            // 决定放置什么方块
            const BlockState* toPlace = basalt;
            if (magma && random.nextFloat() < config.magmaChance) {
                toPlace = magma;
            }

            world.setBlockState(placePos, toPlace);

            // 有时候向下替换一层
            if (random.nextFloat() < 0.3f) {
                BlockPos belowPos(placePos.x, placePos.y - 1, placePos.z);
                const BlockState* belowState = world.getBlockState(belowPos);
                if (belowState && belowState->is(VanillaBlocks::NETHERRACK)) {
                    world.setBlockState(belowPos, toPlace);
                }
            }
        }
    }

    return true;
}

// ============================================================================
// ConfiguredBasaltDeltaFeature 实现
// ============================================================================

ConfiguredBasaltDeltaFeature::ConfiguredBasaltDeltaFeature(
    std::unique_ptr<BasaltDeltaFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredBasaltDeltaFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    (void)chunk;
    (void)generator;
    return m_feature.place(region, random, pos, *m_config);
}

// ============================================================================
// BasaltDeltaFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredBasaltDeltaFeature>> BasaltDeltaFeatures::s_features;

void BasaltDeltaFeatures::initialize()
{
    if (!s_features.empty()) return;
    s_features.push_back(createNormal());
}

const std::vector<std::unique_ptr<ConfiguredBasaltDeltaFeature>>& BasaltDeltaFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredBasaltDeltaFeature>> BasaltDeltaFeatures::getAllFeaturesAndClear()
{
    auto result = std::move(s_features);
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredBasaltDeltaFeature> BasaltDeltaFeatures::createNormal()
{
    auto config = std::make_unique<BasaltDeltaFeatureConfig>(8, // size
        0.2f,                                                   // magmaChance
        true                                                    // useBasalt
    );
    return std::make_unique<ConfiguredBasaltDeltaFeature>(std::move(config), "basalt_delta");
}

} // namespace mc
