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

#include "HugeFungusFeature.hpp"

#include "common/core/Constants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

namespace mc {

// ============================================================================
// HugeFungusFeature 实现
// ============================================================================

bool HugeFungusFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const HugeFungusFeatureConfig& config)
{
    // MC 1.21.11: 检查是否可以放置
    if (!_canPlaceAt(world, pos, config)) {
        return false;
    }

    // MC 1.21.11: 高度 = 4 + random(8) = 4~11
    // 1/12 概率双倍高度
    i32 height = 4 + random.nextInt(8);
    if (random.nextInt(12) == 0) {
        height *= 2;
    }

    // MC 1.21.11: 6% 概率生成粗壮菌柄（3x3 菌柄而非 1x1）
    const bool thickStem = random.nextFloat() < 0.06f;

    // 获取方块状态
    const BlockState* stemState = _getStemState(config.fungusType);
    const BlockState* capState = _getCapState(config.fungusType);
    const BlockState* shroomlightState = VanillaBlocks::getState(VanillaBlocks::SHROOMLIGHT);
    if (!shroomlightState) {
        shroomlightState = VanillaBlocks::getState(VanillaBlocks::GLOWSTONE);
    }
    const BlockState* airState = VanillaBlocks::getState(VanillaBlocks::AIR);

    if (!stemState) return false;

    // MC 1.21.11: 生成菌柄
    _generateStem(world, pos, height, stemState, thickStem);

    // MC 1.21.11: 生成菌盖（在菌柄顶部）
    // 菌盖高度为 1 + random(2)，即 1~2 层
    i32 capHeight = 1 + random.nextInt(2);
    BlockPos capTopPos(pos.x, pos.y + height, pos.z);
    _generateCap(world, random, capTopPos, capHeight, capState, shroomlightState, airState, thickStem);

    // MC 1.21.11: 在菌盖下方生成藤蔓
    _generateVines(world, random, pos, height, config.fungusType);

    // MC 1.21.11: 在菌岩基座下方放置下界岩
    _generateBase(world, pos, config);

    return true;
}

bool HugeFungusFeature::_canPlaceAt(
    WorldGenRegion& world, const BlockPos& pos, const HugeFungusFeatureConfig& config) const
{
    // MC 1.21.11: 检查底部方块是否为菌岩
    const BlockState* groundState = world.getBlockState(pos.x, pos.y - 1, pos.z);
    if (!groundState) return false;

    const Block& ground = groundState->getBlock();
    if (config.fungusType == FungusType::Crimson) {
        // 绯红真菌需要绯红菌岩
        if (VanillaBlocks::CRIMSON_NYLIUM && !groundState->is(VanillaBlocks::CRIMSON_NYLIUM)) {
            // 如果菌岩未注册，回退到下界岩
            if (VanillaBlocks::NETHERRACK && groundState->is(VanillaBlocks::NETHERRACK)) {
                // 允许在下界岩上生成（过渡兼容）
            } else {
                return false;
            }
        }
    } else {
        // 诡异真菌需要诡异菌岩
        if (VanillaBlocks::WARPED_NYLIUM && !groundState->is(VanillaBlocks::WARPED_NYLIUM)) {
            if (VanillaBlocks::NETHERRACK && groundState->is(VanillaBlocks::NETHERRACK)) {
                // 允许在下界岩上生成（过渡兼容）
            } else {
                return false;
            }
        }
    }

    // MC 1.21.11: 如果是种植的，不需要检查空间
    if (config.planted) return true;

    // 检查上方是否有足够空间（检查到高度上限或 16 格）
    for (i32 y = 0; y < 16; ++y) {
        const BlockState* state = world.getBlockState(pos.x, pos.y + y, pos.z);
        if (state && !state->isAir() && !state->isLiquid()) {
            return false;
        }
    }

    return true;
}

const BlockState* HugeFungusFeature::_getStemState(FungusType type) const
{
    if (type == FungusType::Crimson && VanillaBlocks::CRIMSON_STEM) {
        return &VanillaBlocks::CRIMSON_STEM->defaultState();
    }
    if (type == FungusType::Warped && VanillaBlocks::WARPED_STEM) {
        return &VanillaBlocks::WARPED_STEM->defaultState();
    }
    // 回退
    return VanillaBlocks::getState(VanillaBlocks::NETHERRACK);
}

const BlockState* HugeFungusFeature::_getCapState(FungusType type) const
{
    if (type == FungusType::Crimson && VanillaBlocks::NETHER_WART_BLOCK) {
        return &VanillaBlocks::NETHER_WART_BLOCK->defaultState();
    }
    if (type == FungusType::Warped && VanillaBlocks::WARPED_WART_BLOCK) {
        return &VanillaBlocks::WARPED_WART_BLOCK->defaultState();
    }
    // 回退
    return VanillaBlocks::getState(VanillaBlocks::GLOWSTONE);
}

void HugeFungusFeature::_generateStem(
    WorldGenRegion& world, const BlockPos& pos, i32 height, const BlockState* stemState, bool thickStem)
{
    if (!stemState) return;

    if (thickStem) {
        // MC 1.21.11: 粗壮菌柄 - 3x3 截面
        for (i32 y = 0; y < height; ++y) {
            for (i32 dx = -1; dx <= 1; ++dx) {
                for (i32 dz = -1; dz <= 1; ++dz) {
                    world.setBlockState(pos.x + dx, pos.y + y, pos.z + dz, stemState);
                }
            }
        }
    } else {
        // MC 1.21.11: 普通菌柄 - 1x1
        for (i32 y = 0; y < height; ++y) {
            world.setBlockState(pos.x, pos.y + y, pos.z, stemState);
        }
    }
}

void HugeFungusFeature::_generateCap(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& topPos,
    i32 capHeight,
    const BlockState* capState,
    const BlockState* shroomlightState,
    const BlockState* airState,
    bool thickStem)
{
    if (!capState) return;

    // MC 1.21.11: 菌盖是 3x3 或 5x5 的水平区域
    // 对于普通菌柄，菌盖半径为 2（5x5）
    // 对于粗壮菌柄，菌盖半径也为 2（但菌柄本身已占 3x3）
    constexpr i32 CAP_RADIUS = 2;

    for (i32 dy = 0; dy < capHeight; ++dy) {
        i32 y = topPos.y + dy;
        for (i32 dx = -CAP_RADIUS; dx <= CAP_RADIUS; ++dx) {
            for (i32 dz = -CAP_RADIUS; dz <= CAP_RADIUS; ++dz) {
                BlockPos placePos(topPos.x + dx, y, topPos.z + dz);

                // MC 1.21.11: 菌盖内部有随机替换的菌光体
                // MC 的替换概率约为 1/9
                if (shroomlightState && random.nextInt(9) == 0) {
                    // 不替换菌柄位置
                    if (dx == 0 && dz == 0) continue;
                    world.setBlockState(placePos, shroomlightState);
                } else {
                    world.setBlockState(placePos, capState);
                }
            }
        }
    }

    // MC 1.21.11: 菌盖顶层再放一层，但角落有 1/3 概率不放
    {
        i32 y = topPos.y + capHeight;
        for (i32 dx = -CAP_RADIUS; dx <= CAP_RADIUS; ++dx) {
            for (i32 dz = -CAP_RADIUS; dz <= CAP_RADIUS; ++dz) {
                // 角落有 1/3 概率不放
                if ((std::abs(dx) == CAP_RADIUS && std::abs(dz) == CAP_RADIUS) && random.nextInt(3) != 0) {
                    continue;
                }

                BlockPos placePos(topPos.x + dx, y, topPos.z + dz);

                if (shroomlightState && random.nextInt(9) == 0) {
                    if (dx == 0 && dz == 0) continue;
                    world.setBlockState(placePos, shroomlightState);
                } else {
                    world.setBlockState(placePos, capState);
                }
            }
        }
    }
}

void HugeFungusFeature::_generateVines(
    WorldGenRegion& world, math::Random& random, const BlockPos& stemBase, i32 stemHeight, FungusType type)
{
    // MC 1.21.11: 绯红真菌 → 垂泪藤（向下），诡异真菌 → 扭曲藤（向上）
    // 垂泪藤从菌盖底部向下生长
    // 扭曲藤从菌岩向上生长

    if (type == FungusType::Crimson) {
        // 垂泪藤：从菌盖底部向下悬挂
        const BlockState* weepingVines = VanillaBlocks::getState(VanillaBlocks::WEEPING_VINES);
        if (!weepingVines) return;

        // MC 1.21.11: 在菌盖底部的边缘位置随机放置垂泪藤
        constexpr i32 CAP_RADIUS = 2;
        for (i32 dx = -CAP_RADIUS; dx <= CAP_RADIUS; ++dx) {
            for (i32 dz = -CAP_RADIUS; dz <= CAP_RADIUS; ++dz) {
                // 只在边缘位置放藤蔓
                if (std::abs(dx) < CAP_RADIUS && std::abs(dz) < CAP_RADIUS) continue;

                if (random.nextInt(3) != 0) continue;

                i32 vineStartY = stemBase.y + stemHeight - 1;
                i32 vineLength = 1 + random.nextInt(4);

                for (i32 vy = 0; vy < vineLength; ++vy) {
                    BlockPos vinePos(stemBase.x + dx, vineStartY - vy, stemBase.z + dz);
                    const BlockState* existing = world.getBlockState(vinePos);
                    if (existing && existing->isAir()) {
                        world.setBlockState(vinePos, weepingVines);
                    } else {
                        break;
                    }
                }
            }
        }
    } else {
        // 扭曲藤：从菌岩向上生长
        const BlockState* twistingVines = VanillaBlocks::getState(VanillaBlocks::TWISTING_VINES);
        if (!twistingVines) return;

        // MC 1.21.11: 在菌岩基座周围放置扭曲藤
        constexpr i32 VINE_RADIUS = 3;
        i32 vineCount = 2 + random.nextInt(3);
        for (i32 i = 0; i < vineCount; ++i) {
            i32 dx = random.nextInt(VINE_RADIUS * 2 + 1) - VINE_RADIUS;
            i32 dz = random.nextInt(VINE_RADIUS * 2 + 1) - VINE_RADIUS;
            // 避免与菌柄重叠
            if (dx == 0 && dz == 0) continue;

            i32 vineLength = 1 + random.nextInt(4);
            for (i32 vy = 0; vy < vineLength; ++vy) {
                BlockPos vinePos(stemBase.x + dx, stemBase.y + vy, stemBase.z + dz);
                const BlockState* existing = world.getBlockState(vinePos);
                if (existing && existing->isAir()) {
                    world.setBlockState(vinePos, twistingVines);
                } else {
                    break;
                }
            }
        }
    }
}

void HugeFungusFeature::_generateBase(WorldGenRegion& world, const BlockPos& pos, const HugeFungusFeatureConfig& config)
{
    // MC 1.21.11: 将基座下方的方块替换为菌岩
    const BlockState* nyliumState = nullptr;
    if (config.fungusType == FungusType::Crimson && VanillaBlocks::CRIMSON_NYLIUM) {
        nyliumState = &VanillaBlocks::CRIMSON_NYLIUM->defaultState();
    } else if (config.fungusType == FungusType::Warped && VanillaBlocks::WARPED_NYLIUM) {
        nyliumState = &VanillaBlocks::WARPED_NYLIUM->defaultState();
    }

    if (nyliumState) {
        BlockPos basePos(pos.x, pos.y - 1, pos.z);
        const BlockState* current = world.getBlockState(basePos);
        if (current && current->is(VanillaBlocks::NETHERRACK)) {
            world.setBlockState(basePos, nyliumState);
        }
    }
}

// ============================================================================
// ConfiguredHugeFungusFeature 实现
// ============================================================================

ConfiguredHugeFungusFeature::ConfiguredHugeFungusFeature(
    std::unique_ptr<HugeFungusFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredHugeFungusFeature::place(WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos) const
{
    return m_feature.place(region, random, pos, *m_config);
}

} // namespace mc
