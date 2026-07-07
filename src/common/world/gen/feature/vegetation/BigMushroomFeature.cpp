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

#include "BigMushroomFeature.hpp"
#include "../../../../core/Constants.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"

namespace mc {

// ============================================================================
// BigMushroomFeature 实现
// ============================================================================

bool BigMushroomFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const BigMushroomFeatureConfig& config)
{
    // 计算蘑菇高度
    i32 height = calculateHeight(random);

    // 检查是否可以放置
    if (!canPlaceAt(world, pos, height, config)) {
        return false;
    }

    // 生成蘑菇柄
    generateStem(world, random, pos, config, height);

    // 生成蘑菇盖
    generateCap(world, random, pos, height, config);

    return true;
}

void BigMushroomFeature::generateStem(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const BigMushroomFeatureConfig& config,
    i32 height)
{
    MC_UNUSED(random);

    for (i32 y = 0; y < height; ++y) {
        BlockPos stemPos(pos.x, pos.y + y, pos.z);

        // 检查是否可以放置（可被原木替换）
        const BlockState* currentState = world.getBlockState(stemPos);
        if (currentState && !currentState->isAir()) {
            // 蘑菇柄可以替换树叶方块
            if (currentState->is(VanillaBlocks::OAK_LEAVES) || currentState->is(VanillaBlocks::SPRUCE_LEAVES) ||
                currentState->is(VanillaBlocks::BIRCH_LEAVES) || currentState->is(VanillaBlocks::JUNGLE_LEAVES) ||
                currentState->is(VanillaBlocks::ACACIA_LEAVES) || currentState->is(VanillaBlocks::DARK_OAK_LEAVES)) {
                // 允许替换树叶
            } else {
                continue;
            }
        }

        if (config.stemState) {
            world.setBlockState(stemPos, config.stemState);
        }
    }
}

i32 BigMushroomFeature::calculateHeight(math::Random& random) const
{
    // 高度范围: 4-6，有 1/12 概率高度翻倍
    i32 height = random.nextInt(3) + 4;
    if (random.nextInt(12) == 0) {
        height *= 2;
    }
    return height;
}

bool BigMushroomFeature::canPlaceAt(
    WorldGenRegion& world, const BlockPos& pos, i32 height, const BigMushroomFeatureConfig& config) const
{
    MC_UNUSED(config);

    // 检查Y坐标范围
    i32 baseY = pos.y;
    if (baseY < world::MIN_BUILD_HEIGHT + 1 || baseY + height + 1 >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    // 检查下方方块是否为土壤
    const BlockState* belowState = world.getBlockState(BlockPos(pos.x, pos.y - 1, pos.z));
    if (!belowState) {
        return false;
    }

    u32 belowBlockId = belowState->blockId();
    bool isValidGround = belowBlockId == VanillaBlocks::GRASS_BLOCK->blockId() ||
        belowBlockId == VanillaBlocks::DIRT->blockId() || belowBlockId == VanillaBlocks::MYCELIUM->blockId() ||
        belowBlockId == VanillaBlocks::PODZOL->blockId() || belowBlockId == VanillaBlocks::COARSE_DIRT->blockId();

    if (!isValidGround) {
        return false;
    }

    // 检查蘑菇生长空间
    i32 capRadius = config.capRadius;
    for (i32 y = 0; y <= height; ++y) {
        i32 radius = getCapRadius(-1, height, capRadius, y);

        for (i32 dx = -radius; dx <= radius; ++dx) {
            for (i32 dz = -radius; dz <= radius; ++dz) {
                BlockPos checkPos(pos.x + dx, pos.y + y, pos.z + dz);
                const BlockState* state = world.getBlockState(checkPos);

                // 必须为空气或树叶
                if (state && !state->isAir()) {
                    // 允许替换树叶方块
                    if (state->is(VanillaBlocks::OAK_LEAVES) || state->is(VanillaBlocks::SPRUCE_LEAVES) ||
                        state->is(VanillaBlocks::BIRCH_LEAVES) || state->is(VanillaBlocks::JUNGLE_LEAVES) ||
                        state->is(VanillaBlocks::ACACIA_LEAVES) || state->is(VanillaBlocks::DARK_OAK_LEAVES)) {
                        continue;
                    }
                    return false;
                }
            }
        }
    }

    return true;
}

// ============================================================================
// BigBrownMushroomFeature 实现
// ============================================================================

i32 BigBrownMushroomFeature::getCapRadius(i32 baseRadius, i32 totalHeight, i32 capRadius, i32 currentHeight) const
{
    MC_UNUSED(baseRadius);
    MC_UNUSED(totalHeight);

    // 棕色蘑菇：只有顶部有盖
    return currentHeight <= 3 ? 0 : capRadius;
}

void BigBrownMushroomFeature::generateCap(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    i32 height,
    const BigMushroomFeatureConfig& config)
{
    MC_UNUSED(random);

    i32 capRadius = config.capRadius;

    // 生成棕色蘑菇盖，设置正确的方向属性
    for (i32 dx = -capRadius; dx <= capRadius; ++dx) {
        for (i32 dz = -capRadius; dz <= capRadius; ++dz) {
            bool isWest = (dx == -capRadius);
            bool isEast = (dx == capRadius);
            bool isNorth = (dz == -capRadius);
            bool isSouth = (dz == capRadius);
            bool isEdgeX = isWest || isEast;
            bool isEdgeZ = isNorth || isSouth;

            // 跳过四角
            if (isEdgeX && isEdgeZ) {
                continue;
            }

            BlockPos capPos(pos.x + dx, pos.y + height, pos.z + dz);

            // 检查是否可以放置
            const BlockState* currentState = world.getBlockState(capPos);
            if (currentState && !currentState->isAir()) {
                // 允许替换树叶
                if (!currentState->is(VanillaBlocks::OAK_LEAVES) && !currentState->is(VanillaBlocks::SPRUCE_LEAVES) &&
                    !currentState->is(VanillaBlocks::BIRCH_LEAVES) && !currentState->is(VanillaBlocks::JUNGLE_LEAVES) &&
                    !currentState->is(VanillaBlocks::ACACIA_LEAVES) &&
                    !currentState->is(VanillaBlocks::DARK_OAK_LEAVES)) {
                    continue;
                }
            }

            // 计算边缘方向属性
            bool west = isWest || (isEdgeZ && dx == 1 - capRadius);
            bool east = isEast || (isEdgeZ && dx == capRadius - 1);
            bool north = isNorth || (isEdgeX && dz == 1 - capRadius);
            bool south = isSouth || (isEdgeX && dz == capRadius - 1);

            // 放置蘑菇盖，设置方向属性
            if (config.capState) {
                const BlockState* capWithProps = config.capState;
                // 使用 BlockStateProperties 设置属性
                capWithProps = &capWithProps->with(BlockStateProperties::WEST(), west);
                capWithProps = &capWithProps->with(BlockStateProperties::EAST(), east);
                capWithProps = &capWithProps->with(BlockStateProperties::NORTH(), north);
                capWithProps = &capWithProps->with(BlockStateProperties::SOUTH(), south);
                // 棕色蘑菇盖：DOWN=true, UP=false（因为是平顶）
                capWithProps = &capWithProps->with(BlockStateProperties::DOWN(), true);
                capWithProps = &capWithProps->with(BlockStateProperties::UP(), false);
                world.setBlockState(capPos, capWithProps);
            }
        }
    }
}

// ============================================================================
// BigRedMushroomFeature 实现
// ============================================================================

i32 BigRedMushroomFeature::getCapRadius(i32 baseRadius, i32 totalHeight, i32 capRadius, i32 currentHeight) const
{
    MC_UNUSED(baseRadius);

    // 红色蘑菇顶部是 capRadius，下方三层是 capRadius - 1，更早的层没有盖
    if (currentHeight == totalHeight) {
        return capRadius;
    }
    if (currentHeight >= totalHeight - 3 && currentHeight < totalHeight) {
        return capRadius - 1;
    }
    return 0;
}

void BigRedMushroomFeature::generateCap(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    i32 height,
    const BigMushroomFeatureConfig& config)
{
    MC_UNUSED(random);

    i32 capRadius = config.capRadius;
    i32 innerRadius = capRadius - 2;

    // 红色蘑菇盖：多层圆顶形状
    for (i32 y = height - 3; y <= height; ++y) {
        i32 currentRadius = (y < height) ? capRadius : (capRadius - 1);

        for (i32 dx = -currentRadius; dx <= currentRadius; ++dx) {
            for (i32 dz = -currentRadius; dz <= currentRadius; ++dz) {
                bool isWest = (dx == -currentRadius);
                bool isEast = (dx == currentRadius);
                bool isNorth = (dz == -currentRadius);
                bool isSouth = (dz == currentRadius);
                bool isEdgeX = isWest || isEast;
                bool isEdgeZ = isNorth || isSouth;

                // 只在顶层或非对角位置放置
                if (y >= height || isEdgeX != isEdgeZ) {
                    BlockPos capPos(pos.x + dx, pos.y + y, pos.z + dz);

                    // 检查是否可以放置
                    const BlockState* currentState = world.getBlockState(capPos);
                    if (currentState && !currentState->isAir()) {
                        // 允许替换树叶
                        if (!currentState->is(VanillaBlocks::OAK_LEAVES) &&
                            !currentState->is(VanillaBlocks::SPRUCE_LEAVES) &&
                            !currentState->is(VanillaBlocks::BIRCH_LEAVES) &&
                            !currentState->is(VanillaBlocks::JUNGLE_LEAVES) &&
                            !currentState->is(VanillaBlocks::ACACIA_LEAVES) &&
                            !currentState->is(VanillaBlocks::DARK_OAK_LEAVES)) {
                            continue;
                        }
                    }

                    // 计算方向属性
                    bool isUp = (y >= height - 1);
                    bool west = (dx < -innerRadius);
                    bool east = (dx > innerRadius);
                    bool north = (dz < -innerRadius);
                    bool south = (dz > innerRadius);

                    // 放置蘑菇盖，设置方向属性
                    if (config.capState) {
                        const BlockState* capWithProps = config.capState;
                        capWithProps = &capWithProps->with(BlockStateProperties::UP(), isUp);
                        capWithProps = &capWithProps->with(BlockStateProperties::DOWN(), true);
                        capWithProps = &capWithProps->with(BlockStateProperties::WEST(), west);
                        capWithProps = &capWithProps->with(BlockStateProperties::EAST(), east);
                        capWithProps = &capWithProps->with(BlockStateProperties::NORTH(), north);
                        capWithProps = &capWithProps->with(BlockStateProperties::SOUTH(), south);
                        world.setBlockState(capPos, capWithProps);
                    }
                }
            }
        }
    }
}

// ============================================================================
// ConfiguredBigMushroomFeature 实现
// ============================================================================

ConfiguredBigMushroomFeature::ConfiguredBigMushroomFeature(
    std::unique_ptr<BigMushroomFeatureConfig> config, const char* featureName, bool isBrown)
    : m_config(std::move(config))
    , m_name(featureName)
{
    if (isBrown) {
        m_feature = std::make_unique<BigBrownMushroomFeature>();
    } else {
        m_feature = std::make_unique<BigRedMushroomFeature>();
    }
}

bool ConfiguredBigMushroomFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    MC_UNUSED(chunk);
    MC_UNUSED(generator);
    return m_feature->place(region, random, pos, *m_config);
}

} // namespace mc
