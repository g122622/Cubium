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

#include "GlowstoneFeature.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../chunk/IChunkGenerator.hpp"

namespace mc {

// ============================================================================
// GlowstoneFeature 实现
// ============================================================================

bool GlowstoneFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const GlowstoneFeatureConfig& config)
{
    // 检查起始位置是否有效（应该在下界岩或基岩下方）
    const BlockState* state = world.getBlockState(pos);
    if (!state || (!state->is(VanillaBlocks::NETHERRACK) && !state->is(VanillaBlocks::BEDROCK))) {
        return false;
    }

    // 检查下方是否有空间
    const BlockState* belowState = world.getBlockState(pos.x, pos.y - 1, pos.z);
    if (belowState && !belowState->isAir()) {
        return false;
    }

    // 获取萤石方块
    const BlockState* glowstone = VanillaBlocks::getState(VanillaBlocks::GLOWSTONE);
    if (!glowstone) {
        return false;
    }

    // 在起始位置下方放置萤石块
    BlockPos glowPos(pos.x, pos.y - 1, pos.z);
    world.setBlockState(glowPos, glowstone);

    // 生成多个分支
    for (i32 i = 0; i < config.branchCount; ++i) {
        // 随机方向
        i32 dx = random.nextInt(3) - 1; // -1, 0, 1
        i32 dy = random.nextInt(2) - 1; // -1, 0 (向下或水平)
        i32 dz = random.nextInt(3) - 1; // -1, 0, 1

        // 跳过零方向
        if (dx == 0 && dy == 0 && dz == 0) {
            dx = 1;
        }

        i32 branchLength = 1 + random.nextInt(config.maxBranchLength);
        growBranch(world, random, glowPos, dx, dy, dz, branchLength);
    }

    return true;
}

void GlowstoneFeature::growBranch(
    WorldGenRegion& world, math::Random& random, const BlockPos& start, i32 dx, i32 dy, i32 dz, i32 length)
{
    const BlockState* glowstone = VanillaBlocks::getState(VanillaBlocks::GLOWSTONE);
    if (!glowstone) {
        return;
    }

    BlockPos current = start;

    for (i32 i = 0; i < length; ++i) {
        BlockPos next(current.x + dx, current.y + dy, current.z + dz);

        // 边界检查
        if (next.y < 1 || next.y >= 128) {
            break;
        }

        // 检查是否可以放置
        if (!canPlaceAt(world, next)) {
            break;
        }

        // 放置萤石
        world.setBlockState(next, glowstone);

        // 随机添加侧向分支
        if (random.nextInt(4) == 0) {
            i32 sideDx = random.nextInt(3) - 1;
            i32 sideDz = random.nextInt(3) - 1;
            if (sideDx != 0 || sideDz != 0) {
                BlockPos sidePos(next.x + sideDx, next.y, next.z + sideDz);
                if (canPlaceAt(world, sidePos)) {
                    world.setBlockState(sidePos, glowstone);
                }
            }
        }

        current = next;

        // 随机改变方向
        if (random.nextInt(3) == 0) {
            dx = random.nextInt(3) - 1;
            dy = random.nextInt(2) - 1;
            dz = random.nextInt(3) - 1;
            if (dx == 0 && dy == 0 && dz == 0) {
                dy = -1; // 默认向下
            }
        }
    }
}

bool GlowstoneFeature::canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    // 可以在空气或液体中放置
    return !state || state->isAir() || state->isLiquid();
}

// ============================================================================
// ConfiguredGlowstoneFeature 实现
// ============================================================================

ConfiguredGlowstoneFeature::ConfiguredGlowstoneFeature(
    std::unique_ptr<GlowstoneFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredGlowstoneFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
{
    (void)chunk;
    (void)generator;
    return m_feature.place(region, random, pos, *m_config);
}

// ============================================================================
// GlowstoneFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredGlowstoneFeature>> GlowstoneFeatures::s_features;

void GlowstoneFeatures::initialize()
{
    if (!s_features.empty()) return;

    s_features.push_back(createNormal());
    s_features.push_back(createLarge());
}

const std::vector<std::unique_ptr<ConfiguredGlowstoneFeature>>& GlowstoneFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredGlowstoneFeature>> GlowstoneFeatures::getAllFeaturesAndClear()
{
    auto result = std::move(s_features);
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredGlowstoneFeature> GlowstoneFeatures::createNormal()
{
    auto config = std::make_unique<GlowstoneFeatureConfig>(8, // maxDistance
        4,                                                    // branchCount
        6                                                     // maxBranchLength
    );
    return std::make_unique<ConfiguredGlowstoneFeature>(std::move(config), "glowstone");
}

std::unique_ptr<ConfiguredGlowstoneFeature> GlowstoneFeatures::createLarge()
{
    auto config = std::make_unique<GlowstoneFeatureConfig>(12, // maxDistance
        6,                                                     // branchCount
        8                                                      // maxBranchLength
    );
    return std::make_unique<ConfiguredGlowstoneFeature>(std::move(config), "glowstone_large");
}

} // namespace mc
