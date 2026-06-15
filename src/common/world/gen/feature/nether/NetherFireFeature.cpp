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

#include "NetherFireFeature.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "common/world/block/blocks/nether/SoulFireBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"

namespace mc {

// ============================================================================
// NetherFireFeature 实现
// ============================================================================

bool NetherFireFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const NetherFireFeatureConfig& config)
{
    bool placed = false;

    for (i32 i = 0; i < config.spread * config.spread; ++i) {
        i32 dx = random.nextInt(config.spread * 2 + 1) - config.spread;
        i32 dz = random.nextInt(config.spread * 2 + 1) - config.spread;

        BlockPos firePos(pos.x + dx, pos.y, pos.z + dz);

        // 检查位置是否有效（在可燃基座方块上：下界岩、灵魂沙、灵魂土）
        const BlockState* belowState = world.getBlockState(firePos.x, firePos.y - 1, firePos.z);
        if (!belowState ||
            (!belowState->is(VanillaBlocks::NETHERRACK) &&
                !blocks::SoulFireBlock::isSoulFireBase(&belowState->getBlock()))) {
            continue;
        }

        // 检查上方是否有空间
        const BlockState* current = world.getBlockState(firePos);
        if (current && !current->isAir()) {
            continue;
        }

        // 根据下方方块类型选择火焰种类（灵魂沙/灵魂土上方生成灵魂火）
        const BlockState& fireState = blocks::FireBlock::getFireState(world, firePos);
        world.setBlockState(firePos, &fireState);
        placed = true;
    }

    return placed;
}

// ============================================================================
// ConfiguredNetherFireFeature 实现
// ============================================================================

ConfiguredNetherFireFeature::ConfiguredNetherFireFeature(
    std::unique_ptr<NetherFireFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredNetherFireFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    (void)chunk;
    (void)generator;
    return m_feature.place(region, random, pos, *m_config);
}

// ============================================================================
// NetherFireFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredNetherFireFeature>> NetherFireFeatures::s_features;

void NetherFireFeatures::initialize()
{
    if (!s_features.empty()) return;
    s_features.push_back(createNormal());
}

const std::vector<std::unique_ptr<ConfiguredNetherFireFeature>>& NetherFireFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredNetherFireFeature>> NetherFireFeatures::getAllFeaturesAndClear()
{
    auto result = std::move(s_features);
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredNetherFireFeature> NetherFireFeatures::createNormal()
{
    auto config = std::make_unique<NetherFireFeatureConfig>(4, // spread
        1,                                                     // minHeight
        3                                                      // maxHeight
    );
    return std::make_unique<ConfiguredNetherFireFeature>(std::move(config), "nether_fire");
}

} // namespace mc
