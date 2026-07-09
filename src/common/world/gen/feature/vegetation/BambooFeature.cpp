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

#include "BambooFeature.hpp"
#include "common/core/Constants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc {

// ============================================================================
// BambooFeature 实现
// ============================================================================

bool BambooFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const BambooFeatureConfig& config)
{
    if (!config.bambooState) {
        return false;
    }

    // 检查起始位置是否可以放置竹子
    if (!_canPlaceAt(world, pos)) {
        return false;
    }

    // 随机高度 5-16 格，但不能超出建筑高度上限
    const i32 maxHeight = std::min(random.nextInt(12) + 5, world::MAX_BUILD_HEIGHT - pos.y);

    // 以一定概率在竹子基部周围放置灰化土圆盘
    if (random.nextFloat() < config.podzolProbability) {
        const i32 radius = random.nextInt(4) + 1;
        for (i32 dx = -radius; dx <= radius; ++dx) {
            for (i32 dz = -radius; dz <= radius; ++dz) {
                // 圆形范围检测
                if (dx * dx + dz * dz <= radius * radius) {
                    BlockPos podzolPos(pos.x + dx, pos.y - 1, pos.z + dz);
                    const BlockState* groundState = world.getBlockState(podzolPos);
                    if (groundState && groundState->blockId() == VanillaBlocks::DIRT->blockId()) {
                        if (VanillaBlocks::PODZOL) {
                            world.setBlockState(podzolPos, &VanillaBlocks::PODZOL->defaultState());
                        }
                    }
                }
            }
        }
    }

    // 放置竹子主干，记录实际放置的高度
    i32 actualHeight = 0;
    for (i32 y = 0; y < maxHeight; ++y) {
        BlockPos bambooPos(pos.x, pos.y + y, pos.z);
        const BlockState* state = world.getBlockState(bambooPos);
        if (state && !state->isAir()) {
            // 遇到非空气方块，截断竹子高度
            break;
        }
        world.setBlockState(bambooPos, config.bambooState);
        ++actualHeight;
    }

    // 至少放置1格才算成功
    if (actualHeight <= 0) {
        return false;
    }

    // 顶部装饰：如果竹子至少3格高，装饰顶部3格的叶子状态
    // 顶部格：LEAVES=Large, STAGE=1（停止生长）
    // 顶部下方第1格：LEAVES=Large, STAGE=0
    // 顶部下方第2格：LEAVES=Small, STAGE=0
    if (actualHeight >= 3 && config.topFinalState && config.topLargeState && config.topSmallState) {
        // 顶部方块（停止生长，大叶子）
        BlockPos topPos(pos.x, pos.y + actualHeight - 1, pos.z);
        world.setBlockState(topPos, config.topFinalState);

        // 顶部下方第1格（大叶子）
        BlockPos belowTop1(pos.x, pos.y + actualHeight - 2, pos.z);
        world.setBlockState(belowTop1, config.topLargeState);

        // 顶部下方第2格（小叶子）
        BlockPos belowTop2(pos.x, pos.y + actualHeight - 3, pos.z);
        world.setBlockState(belowTop2, config.topSmallState);
    }

    return true;
}

bool BambooFeature::_canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
{
    // 起始位置必须是空气
    const BlockState* state = world.getBlockState(pos);
    if (!state || !state->isAir()) {
        return false;
    }

    // 下方方块必须在 BAMBOO_PLANTABLE_ON 标签中
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    return _isValidGround(world, belowPos);
}

bool BambooFeature::_isValidGround(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return false;
    }

    // 使用 BAMBOO_PLANTABLE_ON 标签检查可种植方块
    const BlockTag& plantableTag = BlockTags::BAMBOO_PLANTABLE_ON();
    return plantableTag.contains(*state);
}

// ============================================================================
// ConfiguredBambooFeature 实现
// ============================================================================

ConfiguredBambooFeature::ConfiguredBambooFeature(std::unique_ptr<BambooFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredBambooFeature::place(WorldGenRegion& region,
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
