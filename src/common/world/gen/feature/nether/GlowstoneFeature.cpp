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
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc {

// ============================================================================
// GlowstoneFeature 实现
// ============================================================================

bool GlowstoneFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const GlowstoneFeatureConfig& config)
{
    (void)config;

    // MC 1.21.11: 检查起始位置是否为空气，上方是否为下界岩/玄武岩/黑石
    const BlockState* state = world.getBlockState(pos);
    if (!state || !state->isAir()) {
        return false;
    }

    const BlockState* aboveState = world.getBlockState(pos.x, pos.y + 1, pos.z);
    if (!aboveState ||
        (!aboveState->is(VanillaBlocks::NETHERRACK) && !aboveState->is(VanillaBlocks::BASALT) &&
            !aboveState->is(VanillaBlocks::BLACKSTONE))) {
        return false;
    }

    // 放置初始萤石块
    const BlockState* glowstone = VanillaBlocks::getState(VanillaBlocks::GLOWSTONE);
    if (!glowstone) {
        return false;
    }
    world.setBlockState(pos, glowstone);

    // MC 1.21.11 扩散算法：迭代 1500 次尝试扩展
    // 每次随机偏移位置，如果该位置是空气且恰好只有 1 个相邻萤石，则放置
    constexpr i32 ITERATIONS = 1500;
    constexpr i32 HORIZONTAL_SPREAD = 8;
    constexpr i32 VERTICAL_DROP = 12;

    for (i32 i = 0; i < ITERATIONS; ++i) {
        // 随机偏移：X/Z 方向 ±8，Y 方向向下 0~11
        i32 dx = random.nextInt(HORIZONTAL_SPREAD) - random.nextInt(HORIZONTAL_SPREAD);
        i32 dy = -random.nextInt(VERTICAL_DROP);
        i32 dz = random.nextInt(HORIZONTAL_SPREAD) - random.nextInt(HORIZONTAL_SPREAD);

        BlockPos candidate(pos.x + dx, pos.y + dy, pos.z + dz);

        // 只在空气中放置
        const BlockState* candidateState = world.getBlockState(candidate);
        if (!candidateState || !candidateState->isAir()) {
            continue;
        }

        // 统计相邻萤石块数量
        i32 adjacentGlowstone = 0;
        constexpr Direction directions[] = {
            Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};

        for (Direction dir : directions) {
            BlockPos neighbor(candidate.x + Directions::xOffset(dir),
                candidate.y + Directions::yOffset(dir),
                candidate.z + Directions::zOffset(dir));

            const BlockState* neighborState = world.getBlockState(neighbor);
            if (neighborState && neighborState->is(VanillaBlocks::GLOWSTONE)) {
                ++adjacentGlowstone;
                // MC 提前退出：超过 1 个相邻萤石则跳过
                if (adjacentGlowstone > 1) {
                    break;
                }
            }
        }

        // 只有恰好 1 个相邻萤石时才放置
        if (adjacentGlowstone == 1) {
            world.setBlockState(candidate, glowstone);
        }
    }

    return true;
}

// ============================================================================
// ConfiguredGlowstoneFeature 实现
// ============================================================================

ConfiguredGlowstoneFeature::ConfiguredGlowstoneFeature(
    std::unique_ptr<GlowstoneFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredGlowstoneFeature::place(WorldGenRegion& region,
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
