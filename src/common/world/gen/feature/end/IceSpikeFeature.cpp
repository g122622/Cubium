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

#include "IceSpikeFeature.hpp"

#include "common/core/Constants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

#include <cmath>

namespace mc {

// ============================================================================
// IceSpikeFeature 实现
// ============================================================================

bool IceSpikeFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const IceSpikeFeatureConfig& config)
{
    // 寻找雪块表面
    BlockPos basePos = pos;
    bool foundSnowBlock = false;

    for (i32 y = pos.y; y >= mc::world::MIN_BUILD_HEIGHT + 1 && !foundSnowBlock; --y) {
        BlockPos checkPos(pos.x, y, pos.z);
        const BlockState* state = world.getBlockState(checkPos);

        if (state && VanillaBlocks::SNOW_BLOCK && state->is(VanillaBlocks::SNOW_BLOCK)) {
            basePos = BlockPos(pos.x, y + 1, pos.z);
            foundSnowBlock = true;
            break;
        }

        if (state && !state->isAir()) {
            break;
        }
    }

    if (!foundSnowBlock) {
        return false;
    }

    // 随机高度调整
    basePos = BlockPos(basePos.x, basePos.y + random.nextInt(4), basePos.z);

    // 计算冰刺高度
    i32 height = random.nextInt(config.maxHeight - 7) + 7;
    i32 baseRadius = height / 4 + random.nextInt(2);

    // 偶尔生成更高的冰刺
    if (baseRadius > 1 && random.nextInt(60) == 0) {
        basePos = BlockPos(basePos.x, basePos.y + 10 + random.nextInt(30), basePos.z);
        height = random.nextInt(20) + 20;
        baseRadius = random.nextInt(2) + 2;
    }

    // 根据类型生成
    if (config.isSpike) {
        _generateSpike(world, random, basePos, height, baseRadius);
    } else {
        _generateIceberg(world, random, basePos, height, baseRadius);
    }

    return true;
}

bool IceSpikeFeature::_canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (!state || !state->isAir()) {
        return false;
    }

    // 检查下方是否为雪块
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    return belowState && VanillaBlocks::SNOW_BLOCK && belowState->is(VanillaBlocks::SNOW_BLOCK);
}

void IceSpikeFeature::_generateSpike(
    WorldGenRegion& world, math::Random& random, const BlockPos& basePos, i32 height, i32 baseRadius)
{
    // 生成尖塔型冰刺，使用浮冰填充

    for (i32 y = 0; y < height; ++y) {
        f32 radiusF = (1.0f - static_cast<f32>(y) / static_cast<f32>(height)) * static_cast<f32>(baseRadius);
        i32 radius = static_cast<i32>(std::ceil(radiusF));

        for (i32 dx = -radius; dx <= radius; ++dx) {
            for (i32 dz = -radius; dz <= radius; ++dz) {
                // 计算到中心的距离
                f32 distX = std::abs(static_cast<f32>(dx)) - 0.25f;
                f32 distZ = std::abs(static_cast<f32>(dz)) - 0.25f;
                f32 distSq = distX * distX + distZ * distZ;
                f32 radiusSq = radiusF * radiusF;

                // 跳过角落
                if (distSq > radiusSq) {
                    continue;
                }

                // 跳过边缘的一些方块（随机）
                if ((dx == -radius || dx == radius || dz == -radius || dz == radius) && random.nextFloat() > 0.75f) {
                    continue;
                }

                // 放置向上的冰刺
                BlockPos spikePos(basePos.x + dx, basePos.y + y, basePos.z + dz);
                const BlockState* currentState = world.getBlockState(spikePos);

                if (currentState &&
                    (currentState->isAir() || currentState->blockId() == VanillaBlocks::DIRT->blockId() ||
                        currentState->blockId() == VanillaBlocks::SNOW->blockId() ||
                        currentState->blockId() == VanillaBlocks::ICE->blockId())) {
                    if (VanillaBlocks::PACKED_ICE) {
                        world.setBlockState(spikePos, &VanillaBlocks::PACKED_ICE->defaultState());
                    }
                }

                // 向下也生成（形成尖顶）
                if (y != 0 && baseRadius > 1) {
                    BlockPos downPos(basePos.x + dx, basePos.y - y, basePos.z + dz);
                    const BlockState* downState = world.getBlockState(downPos);

                    if (downState &&
                        (downState->isAir() || downState->blockId() == VanillaBlocks::DIRT->blockId() ||
                            downState->blockId() == VanillaBlocks::SNOW->blockId() ||
                            downState->blockId() == VanillaBlocks::ICE->blockId())) {
                        if (VanillaBlocks::PACKED_ICE) {
                            world.setBlockState(downPos, &VanillaBlocks::PACKED_ICE->defaultState());
                        }
                    }
                }
            }
        }
    }

    // 生成冰刺底座
    i32 baseExtend = (baseRadius > 1) ? 1 : 0;

    for (i32 dx = -baseExtend; dx <= baseExtend; ++dx) {
        for (i32 dz = -baseExtend; dz <= baseExtend; ++dz) {
            if (dx == 0 && dz == 0) {
                continue;
            }

            BlockPos downPos(basePos.x + dx, basePos.y - 1, basePos.z + dz);
            i32 depth = 50;

            if (std::abs(dx) == 1 && std::abs(dz) == 1) {
                depth = random.nextInt(5);
            }

            while (downPos.y > mc::world::MIN_BUILD_HEIGHT && depth > 0) {
                const BlockState* state = world.getBlockState(downPos);

                if (state && !state->isAir() && state->blockId() != VanillaBlocks::DIRT->blockId() &&
                    state->blockId() != VanillaBlocks::SNOW->blockId() &&
                    state->blockId() != VanillaBlocks::ICE->blockId() &&
                    state->blockId() != VanillaBlocks::PACKED_ICE->blockId()) {
                    break;
                }

                if (VanillaBlocks::PACKED_ICE) {
                    world.setBlockState(downPos, &VanillaBlocks::PACKED_ICE->defaultState());
                }

                downPos = BlockPos(downPos.x, downPos.y - 1, downPos.z);
                --depth;

                if (depth <= 0) {
                    downPos = BlockPos(downPos.x, downPos.y - random.nextInt(5) - 1, downPos.z);
                    depth = random.nextInt(5);
                }
            }
        }
    }
}

void IceSpikeFeature::_generateIceberg(
    WorldGenRegion& world, math::Random& random, const BlockPos& basePos, i32 height, i32 baseRadius)
{
    (void)height;

    // 冰丘型：较矮、较宽的冰结构
    for (i32 y = 0; y < baseRadius * 2; ++y) {
        i32 radius = baseRadius - y / 2;
        if (radius < 1) radius = 1;

        for (i32 dx = -radius; dx <= radius; ++dx) {
            for (i32 dz = -radius; dz <= radius; ++dz) {
                f32 dist = std::sqrt(static_cast<f32>(dx * dx + dz * dz));
                if (dist > static_cast<f32>(radius)) {
                    continue;
                }

                BlockPos icebergPos(basePos.x + dx, basePos.y + y, basePos.z + dz);
                const BlockState* currentState = world.getBlockState(icebergPos);

                if (currentState &&
                    (currentState->isAir() || currentState->blockId() == VanillaBlocks::SNOW->blockId() ||
                        currentState->blockId() == VanillaBlocks::ICE->blockId())) {
                    // 随机使用冰或浮冰
                    if (random.nextFloat() < 0.3f) {
                        if (VanillaBlocks::PACKED_ICE) {
                            world.setBlockState(icebergPos, &VanillaBlocks::PACKED_ICE->defaultState());
                        }
                    } else {
                        if (VanillaBlocks::ICE) {
                            world.setBlockState(icebergPos, &VanillaBlocks::ICE->defaultState());
                        }
                    }
                }
            }
        }
    }
}

// ============================================================================
// ConfiguredIceSpikeFeature 实现
// ============================================================================

ConfiguredIceSpikeFeature::ConfiguredIceSpikeFeature(
    std::unique_ptr<IceSpikeFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredIceSpikeFeature::place(WorldGenRegion& region,
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
