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

    // 参考 MC Java NetherForestVegetationFeature：
    // 使用 spreadHeight 控制垂直偏移范围，使用三角分布使中心区域概率更高。
    // Java: dy = random.nextInt(spreadHeight) - random.nextInt(spreadHeight)
    // C++: dy 范围为 [-minHeight, +maxHeight]，使用三角分布

    for (i32 i = 0; i < config.spread * config.spread; ++i) {
        i32 dx = random.nextInt(config.spread * 2 + 1) - config.spread;
        i32 dz = random.nextInt(config.spread * 2 + 1) - config.spread;

        // 使用三角分布计算 Y 偏移，参考 MC Java NetherForestVegetationFeature
        // Java 使用 spreadHeight 控制对称范围：nextInt(h) - nextInt(h)
        // 此处 minHeight/maxHeight 允许非对称范围：
        //   正向偏移范围 [0, maxHeight]，负向偏移范围 [0, minHeight]
        //   最终 dy = nextInt(maxHeight + 1) - nextInt(minHeight + 1)
        i32 dy = random.nextInt(config.maxHeight + 1) - random.nextInt(config.minHeight + 1);

        BlockPos firePos(pos.x + dx, pos.y + dy, pos.z + dz);

        // 检查位置是否在世界范围内
        if (!world.isWithinWorldBounds(firePos.x, firePos.y, firePos.z)) {
            continue;
        }

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

bool ConfiguredNetherFireFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    (void)chunk;
    (void)generator;
    return m_feature.place(region, random, pos, *m_config);
}

} // namespace mc
